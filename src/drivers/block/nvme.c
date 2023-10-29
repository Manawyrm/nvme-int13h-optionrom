FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <ipxe/xfer.h>
#include <ipxe/uri.h>
#include <ipxe/open.h>
#include <ipxe/interface.h>
#include <ipxe/blockdev.h>
#include <ipxe/edd.h>
#include <ipxe/process.h>
#include <ipxe/malloc.h>
#include <ipxe/dma.h>
#include <ipxe/io.h>
#include <ipxe/efi/efi_path.h>
#include <ipxe/blocktrans.h>

#include "nvme.h"
#include "nvme-int.h"

/** @file
 *
 * NVMe storage driver
 *
 */

/** List of NVMe devices */
static LIST_HEAD ( nvme_devices );

// Page aligned "dma bounce buffer" of size NVME_PAGE_SIZE in high memory
static void *nvme_dma_buffer;

struct dma_mapping sqe_map;
struct dma_mapping cqe_map;
struct dma_mapping identify_map;

static void * zalloc_page_aligned(u32 size)
{
    void *res = malloc_phys(size, NVME_PAGE_SIZE);
    if (res) memset(res, 0, size);
    return res;
}

/******************************************************************************
 *
 * Endpoint management
 *
 ******************************************************************************
 */
static void nvme_init_queue_common(struct nvme_ctrl *ctrl, volatile struct nvme_queue *q, u16 q_idx,
                                   u16 length)
{
    memset(q, 0, sizeof(*q));
    q->dbl = (u32 *)((char *)ctrl->reg + 0x1000 + q_idx * ctrl->doorbell_stride);

    DBGC ( ctrl, "nvme_init_queue_common(%p), stride: %p\n", ctrl->reg, ctrl->doorbell_stride);
    DBGC ( ctrl, " q %p q_idx %d dbl %p\n", q, q_idx, q->dbl);
    q->mask = length - 1;
    mb();
}


/* Waits for CSTS.RDY to match rdy. Returns 0 on success. */
static int nvme_wait_csts_rdy(struct nvme_ctrl *ctrl, unsigned rdy)
{
//    u32 const max_to = 500 /* ms */ * ((ctrl->reg->cap >> 24) & 0xFFU);
//    u32 to = timer_calc(max_to);
    u32 csts;
    mb();
    while (rdy != ((csts = ctrl->reg->csts) & NVME_CSTS_RDY)) {
        mb();
//        yield();

//        if (csts & NVME_CSTS_FATAL) {
//            DBGC ( ctrl, "NVMe fatal error during controller shutdown\n");
//            return -1;
//        }
//
//        if (timer_check(to)) {
//            warn_timeout();
//            return -1;
//        }
    }

    return 0;
}

static int nvme_init_sq(struct nvme_ctrl *ctrl, volatile struct nvme_sq *sq, u16 q_idx, u16 length,
                        volatile struct nvme_cq *cq)
{
    nvme_init_queue_common(ctrl, &sq->common, q_idx, length);
    sq->sqe = dma_alloc ( &ctrl->pci->dma, &sqe_map, sizeof(*sq->sqe) * length, 4096 );
    memset(sq->sqe, 0, sizeof(*sq->sqe) * length);

    if (!sq->sqe) {
        DBGC ( ctrl, "Failed to alloc memory!\n");
        return -1;
    }

    DBGC ( ctrl, "sq %p q_idx %d sqe %p\n", sq, q_idx, sq->sqe);
    sq->cq   = cq;
    sq->head = 0;
    sq->tail = 0;

    return 0;
}

static int nvme_init_cq(struct nvme_ctrl *ctrl, volatile struct nvme_cq *cq, u16 q_idx, u16 length)
{
    nvme_init_queue_common(ctrl, &cq->common, q_idx, length);
    cq->cqe = dma_alloc ( &ctrl->pci->dma, &cqe_map, sizeof(*cq->cqe) * length, 4096 );
    memset(cq->cqe, 0, sizeof(*cq->cqe) * length);

    if (!cq->cqe) {
        DBGC ( ctrl, "Failed to alloc memory!\n");
        return -1;
    }

    cq->head = 0;

    /* All CQE phase bits are initialized to zero. This means initially we wait
       for the host controller to set these to 1. */
    cq->phase = 1;

    return 0;
}

/* Returns the next submission queue entry (or NULL if the queue is full). It
   also fills out Command Dword 0 and clears the rest. */
static volatile struct nvme_sqe * nvme_get_next_sqe(volatile struct nvme_sq *sq, u8 opc, void *metadata, void *data, void *data2)
{
    if (((sq->head + 1) & sq->common.mask) == sq->tail) {
        DBGC ( sq, "submission queue is full\n");
        return NULL;
    }

    volatile struct nvme_sqe *sqe = &sq->sqe[sq->tail];
    //DBGC ( sq, "sq %p next_sqe %d\n", sq, sq->tail);

    memset(sqe, 0, sizeof(*sqe));
    sqe->cdw0 = opc | (sq->tail << 16 /* CID */);
    sqe->mptr = (u32)metadata;
    sqe->dptr_prp1 = (u32)data;
    sqe->dptr_prp2 = (u32)data2;

    //DBGC ( sq, "sqe->dptr_prp1 %p\n", sqe->dptr_prp1);
    mb();
    return sqe;
}

static int nvme_poll_cq(volatile struct nvme_cq *cq)
{
    //DBGC ( cq, "nvme_poll_cq %p\n", &cq->cqe[cq->head].dword[3]);
    //u32 dw3 = (cq->cqe[cq->head].dword[3]);
    mb();
    u32 dw3 = cq->cqe[cq->head].dword[3];

    return (!!(dw3 & NVME_CQE_DW3_P) == cq->phase);
}

static int nvme_is_cqe_success(volatile struct nvme_cqe const *cqe)
{
    return ((cqe->status >> 1) & 0xFF) == 0;
}

static volatile struct nvme_cqe nvme_error_cqe(void)
{
    volatile struct nvme_cqe r;

    /* 0xFF is a vendor specific status code != success. Should be okay for
       indicating failure. */
    memset(&r, 0xFF, sizeof(r));
    return r;
}

static volatile struct nvme_cqe nvme_consume_cqe(volatile struct nvme_sq *sq)
{
    volatile struct nvme_cq *cq = sq->cq;
    mb();
    if (!nvme_poll_cq(cq)) {
        /* Cannot consume a completion queue entry, if there is none ready. */
        DBGC ( sq, "Cannot consume a completion queue entry, if there is none ready: %p\n", cq);
        return nvme_error_cqe();
    }

    mb();
    struct nvme_cqe cqe = cq->cqe[cq->head];
    u16 cq_next_head = (cq->head + 1) & cq->common.mask;
    mb();

    //DBGC ( sq, "cq %p head %d -> %d\n", cq, cq->head, cq_next_head);
    //udelay(100);
    mb();

    if (cq_next_head < cq->head) {
        //DBGC ( sq, "cq %p wrap\n", cq);
        cq->phase = ~cq->phase;
    }
    cq->head = cq_next_head;
    mb();

    /* Update the submission queue head. */
    if (cqe.sq_head != sq->head) {
        sq->head = cqe.sq_head;
        //DBGC ( sq, "sq %p advanced to %d\n", sq, cqe->sq_head);
    }


    /* Tell the controller that we consumed the completion. */
    cq->common.dbl[0] = cq->head;
    mb();

    return cqe;
}

/* Call this after you've filled out an sqe that you've got from nvme_get_next_sqe. */
static void nvme_commit_sqe(volatile struct nvme_sq *sq)
{
    //DBGC ( sq, "sq %p commit_sqe %d doorbell address: %p\n", sq, sq->tail, sq->common.dbl);
    sq->tail = (sq->tail + 1) & sq->common.mask;

    sq->common.dbl[0] = sq->tail;
    mb();
}

static struct nvme_cqe nvme_wait(volatile struct nvme_sq *sq)
{
    static const unsigned nvme_timeout = 5000 /* ms */;
    //u32 to = timer_calc(nvme_timeout);
    mb();
    while (!nvme_poll_cq(sq->cq)) {
        mb();
        //yield();

        //if (timer_check(to)) {
        //    warn_timeout();
        //    return nvme_error_cqe();
        //}
        //DBGC ( sq, "nvme_wait()\n");
    }

    return nvme_consume_cqe(sq);
}

/* Perform an identify command on the admin queue and return the resulting
   buffer. This may be a NULL pointer, if something failed. This function
   cannot be used after initialization, because it uses buffers in tmp zone. */
volatile static union nvme_identify * nvme_admin_identify(struct nvme_ctrl *ctrl, u8 cns, u32 nsid)
{
    union nvme_identify *identify_buf = dma_alloc ( &ctrl->pci->dma, &identify_map, 4096, 4096 );
    if (!identify_buf) {
        /* Could not allocate identify buffer. */
        DBGC ( ctrl, "Could not allocate identify buffer!\n");
        return NULL;
    }
    memset(identify_buf, 0, 4096);

    DBGC ( ctrl, "nvme_get_next_sqe(&ctrl->admin_sq\n");

    volatile struct nvme_sqe *cmd_identify;
    cmd_identify = nvme_get_next_sqe(&ctrl->admin_sq,
                                     NVME_SQE_OPC_ADMIN_IDENTIFY, NULL,
                                     dma(&identify_map, identify_buf), NULL);

    if (!cmd_identify) {
        DBGC ( ctrl, "!cmd_identify!\n");
        goto error;
    }

    cmd_identify->nsid = nsid;
    cmd_identify->dword[10] = cns;
    nvme_commit_sqe(&ctrl->admin_sq);
    struct nvme_cqe cqe = nvme_wait(&ctrl->admin_sq);
    if (!nvme_is_cqe_success(&cqe)) {
        goto error;
    }

    return identify_buf;
    error:
    dma_free(&identify_map, identify_buf, 4096);
    return NULL;
}

static volatile struct nvme_identify_ctrl * nvme_admin_identify_ctrl(struct nvme_ctrl *ctrl)
{
    return &nvme_admin_identify(ctrl, NVME_ADMIN_IDENTIFY_CNS_ID_CTRL, 0)->ctrl;
}

static volatile struct nvme_identify_ns * nvme_admin_identify_ns(struct nvme_ctrl *ctrl, u32 ns_id)
{
    return &nvme_admin_identify(ctrl, NVME_ADMIN_IDENTIFY_CNS_ID_NS,
                                ns_id)->ns;
}

/* Release memory allocated for a completion queue */
static void nvme_destroy_cq(volatile struct nvme_cq *cq)
{
    free(cq->cqe);
    cq->cqe = NULL;
}

/* Release memory allocated for a submission queue */
static void nvme_destroy_sq(volatile struct nvme_sq *sq)
{
    free(sq->sqe);
    sq->sqe = NULL;
}

/* Returns 0 on success. */
static int nvme_create_io_cq(struct nvme_ctrl *ctrl, volatile struct nvme_cq *cq, u16 q_idx)
{
    int rc;
    struct nvme_sqe *cmd_create_cq;
    u32 length = 1 + (ctrl->reg->cap & 0xffff);
    if (length > NVME_PAGE_SIZE / sizeof(struct nvme_cqe))
        length = NVME_PAGE_SIZE / sizeof(struct nvme_cqe);

    rc = nvme_init_cq(ctrl, cq, q_idx, length);
    if (rc) {
        goto err;
    }

    // FIXME: virt_to_phys oder DMA Buffer?!

    cmd_create_cq = nvme_get_next_sqe(&ctrl->admin_sq,
                                      NVME_SQE_OPC_ADMIN_CREATE_IO_CQ, NULL,
                                      virt_to_phys(cq->cqe), NULL);
    if (!cmd_create_cq) {
        goto err_destroy_cq;
    }

    cmd_create_cq->dword[10] = (cq->common.mask << 16) | (q_idx >> 1);
    cmd_create_cq->dword[11] = 1 /* physically contiguous */;

    nvme_commit_sqe(&ctrl->admin_sq);
    struct nvme_cqe cqe = nvme_wait(&ctrl->admin_sq);

    if (!nvme_is_cqe_success(&cqe)) {
        DBGC ( ctrl, "create io cq failed: %08x %08x %08x %08x\n",
               cqe.dword[0], cqe.dword[1], cqe.dword[2], cqe.dword[3]);

        goto err_destroy_cq;
    }

    return 0;

    err_destroy_cq:
    nvme_destroy_cq(cq);
    err:
    return -1;
}

/* Returns 0 on success. */
static int nvme_create_io_sq(struct nvme_ctrl *ctrl, volatile struct nvme_sq *sq, u16 q_idx, struct nvme_cq *cq)
{
    int rc;
    struct nvme_sqe *cmd_create_sq;
    u32 length = 1 + (ctrl->reg->cap & 0xffff);
    if (length > NVME_PAGE_SIZE / sizeof(struct nvme_cqe))
        length = NVME_PAGE_SIZE / sizeof(struct nvme_cqe);

    rc = nvme_init_sq(ctrl, sq, q_idx, length, cq);
    if (rc) {
        goto err;
    }

    // FIXME: virt_to_phys oder DMA Buffer?!

    cmd_create_sq = nvme_get_next_sqe(&ctrl->admin_sq,
                                      NVME_SQE_OPC_ADMIN_CREATE_IO_SQ, NULL,
                                      virt_to_phys(sq->sqe), NULL);
    if (!cmd_create_sq) {
        goto err_destroy_sq;
    }

    cmd_create_sq->dword[10] = (sq->common.mask << 16) | (q_idx >> 1);
    cmd_create_sq->dword[11] = (q_idx >> 1) << 16 | 1 /* contiguous */;
    DBGC ( ctrl, "sq %p create dword10 %08x dword11 %08x\n", sq,
           cmd_create_sq->dword[10], cmd_create_sq->dword[11]);

    nvme_commit_sqe(&ctrl->admin_sq);
    struct nvme_cqe cqe = nvme_wait(&ctrl->admin_sq);

    if (!nvme_is_cqe_success(&cqe)) {
        DBGC ( ctrl, "create io sq failed: %08x %08x %08x %08x\n",
               cqe.dword[0], cqe.dword[1], cqe.dword[2], cqe.dword[3]);
        goto err_destroy_sq;
    }

    return 0;

    err_destroy_sq:
    nvme_destroy_sq(sq);
    err:
    return -1;
}

static int nvme_create_io_queues(struct nvme_ctrl *ctrl)
{
    if (nvme_create_io_cq(ctrl, &ctrl->io_cq, 3))
        goto err;

    if (nvme_create_io_sq(ctrl, &ctrl->io_sq, 2, &ctrl->io_cq))
        goto err_free_cq;

    return 0;

    err_free_cq:
    nvme_destroy_cq(&ctrl->io_cq);
    err:
    return -1;
}

static void nvme_probe_ns(struct nvme_ctrl *ctrl, u32 ns_idx, u8 mdts)
{
    u32 ns_id = ns_idx + 1;

    volatile struct nvme_identify_ns *id = nvme_admin_identify_ns(ctrl, ns_id);
    if (!id) {
        DBGC ( ctrl, "NVMe couldn't identify namespace %d.\n", ns_id);
        goto free_buffer;
    }

    DBG_HDA_IF( LOG, 0, id, sizeof(struct nvme_identify_ns) );

    u8 current_lba_format = id->flbas & 0xF;
    if (current_lba_format > id->nlbaf) {
        DBGC ( ctrl, "NVMe NS %d: current LBA format %d is beyond what the "
                     " namespace supports (%d)?\n",
               ns_id, current_lba_format, id->nlbaf + 1);
        goto free_buffer;
    }

    if (!id->nsze) {
        DBGC ( ctrl, "NVMe NS %d is inactive.\n", ns_id);
        goto free_buffer;
    }

    if (!nvme_dma_buffer) {
        nvme_dma_buffer = dma_alloc ( &ctrl->pci->dma, &sqe_map, NVME_PAGE_SIZE, NVME_PAGE_SIZE );
        if (!nvme_dma_buffer) {
            DBGC ( ctrl, "Failed to allocate NVMe DMA buffer\n");
            goto free_buffer;
        }
        memset(nvme_dma_buffer, 0, NVME_PAGE_SIZE);
    }

    struct nvme_namespace *ns = malloc(sizeof(*ns));
    if (!ns) {
        DBGC ( ctrl, "ns could not be allocated.\n");
        goto free_buffer;
    }
    memset(ns, 0, sizeof(*ns));
    ns->ctrl  = ctrl;
    ns->ns_id = ns_id;
    ns->lba_count = id->nsze;

    struct nvme_lba_format *fmt = &id->lbaf[current_lba_format];

    ns->block_size    = 1U << fmt->lbads;
    ns->metadata_size = fmt->ms;

    if (ns->block_size > NVME_PAGE_SIZE) {
        /* If we see devices that trigger this path, we need to increase our
           buffer size. */
        DBGC ( ctrl, "If we see devices that trigger this path, we need to increase our\n"
                     "           buffer size.\n",
               ns_id, ns->max_req_size);
        free(ns);
        goto free_buffer;
    }

    if (mdts) {
        ns->max_req_size = ((1U << mdts) * NVME_PAGE_SIZE) / ns->block_size;
        DBGC ( ctrl, "NVME NS %d max request size: %d sectors\n",
               ns_id, ns->max_req_size);
    } else {
        ns->max_req_size = -1U;
    }

    DBGC ( ctrl,"NVMe NS %d: ", ns_id);
    DBGC ( ctrl,"%d MiB ", (ns->lba_count * ns->block_size) >> 20);
    DBGC ( ctrl, "(%d", ns->lba_count);
    DBGC ( ctrl, " %d-byte ", ns->block_size);
    DBGC ( ctrl, "blocks + %d-byte metadata)\n", ns->metadata_size);

    ctrl->ns = ns;

    free_buffer:
    dma_free(&identify_map, id, 4096);
}

static int nvme_controller_enable(struct nvme_ctrl *ctrl)
{
    int rc;

    /* Turn the controller off. */
    ctrl->reg->cc = 0;
    if (nvme_wait_csts_rdy(ctrl, 0)) {
        DBGC ( ctrl, "NVMe fatal error during controller shutdown\n");
        return -1;
    }

    ctrl->doorbell_stride = 4U << ((ctrl->reg->cap >> 32) & 0xF);

    rc = nvme_init_cq(ctrl, &ctrl->admin_cq, 1,
                      NVME_PAGE_SIZE / sizeof(struct nvme_cqe));
    if (rc) {
        return -1;
    }

    rc = nvme_init_sq(ctrl, &ctrl->admin_sq, 0,
                      NVME_PAGE_SIZE / sizeof(struct nvme_sqe), &ctrl->admin_cq);
    if (rc) {
        goto err_destroy_admin_cq;
    }

    ctrl->reg->aqa = ctrl->admin_cq.common.mask << 16
                     | ctrl->admin_sq.common.mask;

    ctrl->reg->asq = dma(&sqe_map, ctrl->admin_sq.sqe);
    ctrl->reg->acq = dma(&cqe_map, ctrl->admin_cq.cqe);

    ctrl->reg->cc = NVME_CC_EN | (NVME_CQE_SIZE_LOG << 20)
                    | (NVME_SQE_SIZE_LOG << 16 /* IOSQES */);

    if (nvme_wait_csts_rdy(ctrl, 1)) {
        DBGC ( ctrl, "NVMe fatal error while enabling controller\n");
        goto err_destroy_admin_sq;
    }

    /* The admin queue is set up and the controller is ready. Let's figure out
       what namespaces we have. */

    volatile struct nvme_identify_ctrl *identify = nvme_admin_identify_ctrl(ctrl);

    if (!identify) {
        DBGC ( ctrl, "NVMe couldn't identify controller.\n");
        goto err_destroy_admin_sq;
    }

    DBGC ( ctrl, "NVMe has %d namespace%s.\n",
           identify->nn, (identify->nn == 1) ? "" : "s");

    DBGC ( ctrl, "NVMe has %d namespace%s.\n",
           identify->nn, (identify->nn == 1) ? "" : "s");

    ctrl->ns_count = identify->nn;
    u8 mdts = identify->mdts;
    dma_free(&identify_map, identify, 4096);

    if ((ctrl->ns_count == 0) || nvme_create_io_queues(ctrl)) {
        /* No point to continue, if the controller says it doesn't have
           namespaces, or we couldn't create I/O queues. */
        goto err_destroy_admin_sq;
    }

    // FIXME: hack
    ctrl->ns_count = 1;

    /* Populate namespace IDs */
    int ns_idx;
    for (ns_idx = 0; ns_idx < ctrl->ns_count; ns_idx++) {
        nvme_probe_ns(ctrl, ns_idx, mdts);
    }

    DBGC ( ctrl, "NVMe initialization complete!\n");
    return 0;

    err_destroy_admin_sq:
    nvme_destroy_sq(&ctrl->admin_sq);
    err_destroy_admin_cq:
    nvme_destroy_cq(&ctrl->admin_cq);
    return -1;
}

/* Reads count sectors into buf. The buffer cannot cross page boundaries. */
static int nvme_io_xfer(struct nvme_namespace *ns, u64 lba, void *prp1, void *prp2,
                        u16 count, int write)
{
    if (((u32)prp1 & 0x3) || ((u32)prp2 & 0x3)) {
        /* Buffer is misaligned */
        DBGC ( ns, "Internal buffer is misaligned.\n");
        return -EBUSY;
    }
    retry:
    volatile struct nvme_sqe *io_read = nvme_get_next_sqe(&ns->ctrl->io_sq,
                                                          write ? NVME_SQE_OPC_IO_WRITE
                                                                : NVME_SQE_OPC_IO_READ,
                                                          NULL, prp1, prp2);
    io_read->nsid = ns->ns_id;
    io_read->dword[10] = (u32)lba;
    io_read->dword[11] = (u32)(lba >> 32);
    io_read->dword[12] = (1U << 31 /* limited retry */) | (count - 1);

    nvme_commit_sqe(&ns->ctrl->io_sq);

    struct nvme_cqe cqe = nvme_wait(&ns->ctrl->io_sq);
    // DBGC ( ns, "ns %d %s lba %d+%d\n", ns->ns_id, write ? "write" : "read", lba, count);

    if (!nvme_is_cqe_success(&cqe)) {
        DBGC ( ns, "read error: %08x %08x %08x %08x\n",
               cqe.dword[0], cqe.dword[1], cqe.dword[2], cqe.dword[3]);

        goto retry;
        return -1;
    }

    return count;
}

// Transfer up to one page of data using the internal dma bounce buffer
static int nvme_bounce_xfer(struct nvme_namespace *ns, u64 lba, void *buf, u16 count,
                            int write)
{
    u16 const max_blocks = NVME_PAGE_SIZE / ns->block_size;
    u16 blocks = count < max_blocks ? count : max_blocks;

    if (write)
        memcpy(nvme_dma_buffer, buf, blocks * ns->block_size);

    int res = nvme_io_xfer(ns, lba, nvme_dma_buffer, NULL, blocks, write);

    if (!write && res >= 0)
        memcpy(buf, nvme_dma_buffer, res * ns->block_size);

    return res;
}

#define NVME_MAX_PRPL_ENTRIES 15 /* Allows requests up to 64kb */

// Transfer data using page list (if applicable)
static int nvme_prpl_xfer(struct nvme_namespace *ns, u64 lba, void *buf, u16 count,
                          int write)
{
    u32 base = (long)buf;
    s32 size;

    if (count > ns->max_req_size)
        count = ns->max_req_size;

    size = count * ns->block_size;
    /* Special case for transfers that fit into PRP1, but are unaligned */
    if (((size + (base & ~NVME_PAGE_MASK)) <= NVME_PAGE_SIZE))
        goto single;

    /* Every request has to be page aligned */
    if (base & ~NVME_PAGE_MASK)
        goto bounce;

    /* Make sure a full block fits into the last chunk */
    if (size & (ns->block_size - 1ULL))
        goto bounce;

    /* Build PRP list if we need to describe more than 2 pages */
    if ((ns->block_size * count) > (NVME_PAGE_SIZE * 2)) {
        u32 prpl_len = 0;
        u64 *prpl = nvme_dma_buffer;
        int first_page = 1;
        for (; size > 0; base += NVME_PAGE_SIZE, size -= NVME_PAGE_SIZE) {
            if (first_page) {
                /* First page is special */
                first_page = 0;
                continue;
            }
            if (prpl_len >= NVME_MAX_PRPL_ENTRIES)
                goto bounce;
            prpl[prpl_len++] = base;
        }
        return nvme_io_xfer(ns, lba, virt_to_phys(buf), virt_to_phys(prpl), count, write);
    }

    /* Directly embed the 2nd page if we only need 2 pages */
    if ((ns->block_size * count) > NVME_PAGE_SIZE)
        return nvme_io_xfer(ns, lba, virt_to_phys(buf), virt_to_phys(buf + NVME_PAGE_SIZE), count, write);

    single:
    /* One page is enough, don't expose anything else */
    return nvme_io_xfer(ns, lba, virt_to_phys(buf), NULL, count, write);

    bounce:
    /* Use bounce buffer to make transfer */
    return nvme_bounce_xfer(ns, lba, virt_to_phys(buf), count, write);
}

//static int nvme_cmd_readwrite(struct nvme_namespace *ns, struct disk_op_s *op, int write)
//{
//    int i;
//    for (i = 0; i < op->count;) {
//        u16 blocks_remaining = op->count - i;
//        char *op_buf = op->buf_fl + i * ns->block_size;
//        int blocks = nvme_prpl_xfer(ns, op->lba + i, op_buf,
//                                    blocks_remaining, write);
//        if (blocks < 0)
//            return DISK_RET_EBADTRACK;
//        i += blocks;
//    }
//
//    return DISK_RET_SUCCESS;
//}

static void nvme_close ( struct nvme_device *nvme, int rc ) {
    DBGC ( nvme, PCI_FMT " nvme_close()\n", PCI_ARGS ( &nvme->pci_dev ) );
    intf_shutdown ( &nvme->block, -ENODEV );
    nvme->opened = 0;
    //list_del ( &usbblk->list );
    //nvme_scsi_close ( usbblk, -ENODEV );
    //ref_put ( &usbblk->refcnt );

    return;
}

struct interface dummy;
uint8_t busy = 0;

static void nvme_step ( struct nvme_device *nvme ) {
    /* Shut down interfaces */
    //DBGC ( nvme, PCI_FMT " nvme_step calls intfs_shutdown(%p) \n", PCI_ARGS ( &nvme->pci_dev ), &nvme->block );
    //intfs_shutdown ( 0, &nvme->block, NULL );
    intfs_shutdown ( 0, &dummy, NULL );

    process_del(&nvme->process);
    ref_put(&nvme->refcnt);
    busy = 0;
}

#define LOGICAL_PAGE_SIZE 512

static int nvme_read ( struct nvme_device *nvme,
                       struct interface *block,
                       uint64_t lba, unsigned int count,
                       userptr_t buffer, size_t len ) {
    //DBGC ( nvme, PCI_FMT " nvme_read()\n", PCI_ARGS ( &nvme->pci_dev ) );
    if (busy)
    {
        DBGC( nvme, "busy!\n");
        return -EBUSY;
    }

    if ( !nvme->ctrl->ns )
    {
        DBGC( nvme, "nvme->ctrl->ns!\n");
        ref_get(&nvme->refcnt);
        return -EBUSY;
    }

    if ( count != 1 )
    {
        DBGC( nvme, "nvme_read() got count != 1, not supported yet!\n");
        ref_get(&nvme->refcnt);
        return -EBUSY;
    }

    //DBGC( nvme, "lba: %d, ", lba);
    //DBGC( nvme, "len: %d \n", len);

    if ( len % nvme->ctrl->ns->block_size != 0 )
    {
//        uint64_t address = lba * LOGICAL_PAGE_SIZE;
//        uint64_t physical_block = (address / nvme->ctrl->ns->block_size);
//        uint64_t block_offset = (address % nvme->ctrl->ns->block_size);
//        DBGC( nvme, "address: %d, ", address);
//        DBGC( nvme, "phys: %d, ", physical_block);
//        DBGC( nvme, "offset: %d\n", block_offset);
//
//        DBGC( nvme, "lba: %d, ", lba);
//        DBGC( nvme, "len: %d \n", len);
//
//        int res = nvme_io_xfer(nvme->ctrl->ns, physical_block, virt_to_phys(nvme_dma_buffer), NULL, 1, 0);
//        memcpy(user_to_virt(buffer, 0), nvme_dma_buffer + block_offset, len);
        DBGC( nvme, "len % nvme->ctrl->ns->block_size != 0!\n");
        ref_get(&nvme->refcnt);
        return -EBUSY;
    }
    else
    {
        int res = nvme_io_xfer(nvme->ctrl->ns, lba, virt_to_phys(nvme_dma_buffer), NULL, 1, 0);
        copy_to_user ( buffer, 0, nvme_dma_buffer, 512 );

        //DBG_HDA_IF( LOG, 0, user_to_virt(buffer, 0), 512 );
        //nvme_prpl_xfer(nvme->ctrl->ns, lba, user_to_virt(buffer, 0), len / NVME_PAGE_SIZE,  0);
    }

    process_add ( &nvme->process );
    intf_plug_plug ( &dummy, block );
    ref_get(&nvme->refcnt);
    busy = 1;
    return 0;
}

static int nvme_write ( struct nvme_device *nvme,
                        struct interface *block,
                        uint64_t lba, unsigned int count,
                        userptr_t buffer, size_t len ) {
    DBGC ( nvme, PCI_FMT " nvme_write()\n", PCI_ARGS ( &nvme->pci_dev ) );
    process_add ( &nvme->process );
    intf_plug_plug ( &dummy, block );
    ref_get(&nvme->refcnt);
    busy = 1;
    return 0;
    // return atadev_command ( atadev, block, &atacmd_write,
    //                         lba, count, buffer, len );
}

/** NVMe process descriptor */
static struct process_descriptor nvme_process_desc =
        PROC_DESC_ONCE ( struct nvme_device, process, nvme_step );


static int nvme_read_capacity ( struct nvme_device *nvme,
                                struct interface *block ) {
    DBGC ( nvme, PCI_FMT " nvme_read_capacity()\n", PCI_ARGS ( &nvme->pci_dev ) );
    struct block_device_capacity capacity;

    if ( !nvme->ctrl->ns )
    {
        ref_get(&nvme->refcnt);
        return -EBUSY;
    }

    process_add ( &nvme->process );

    //capacity.blocks = nvme->ctrl->ns->lba_count * 8;
    //capacity.blksize = LOGICAL_PAGE_SIZE;

    capacity.blocks = nvme->ctrl->ns->lba_count;
    //capacity.blocks = 268435456UL;
    capacity.blksize = nvme->ctrl->ns->block_size;
    capacity.max_count = 1;

    intf_plug_plug ( &dummy, block );

    /* Return capacity to caller */
    block_capacity ( &dummy, &capacity );

    ref_get(&nvme->refcnt);

    /* Shut down interfaces */
    //intfs_shutdown ( 0, &dummy, NULL );


    return 0;
    //return atadev_command ( atadev, block, &atacmd_identify,
    //                        0, 1, UNULL, ATA_SECTOR_SIZE );
}

static int nvme_edd_describe ( struct nvme_device *nvme,
                               struct edd_interface_type *type,
                               union edd_device_path *path ) {

    DBGC ( nvme, PCI_FMT " nvme_edd_describe()\n", PCI_ARGS ( &nvme->pci_dev ) );

    type->type = cpu_to_le64 ( EDD_INTF_TYPE_RAID );
    return 0;
}
/******************************************************************************
 *
 * SAN device interface
 *
 ******************************************************************************
 */

/** NVMe block interface operations */
static struct interface_operation nvme_block_op[] = {
        INTF_OP ( block_read, struct nvme_device *, nvme_read ),
        INTF_OP ( block_write, struct nvme_device *, nvme_write ),
        INTF_OP ( block_read_capacity, struct nvme_device *,
                  nvme_read_capacity ),
        INTF_OP ( intf_close, struct nvme_device *, nvme_close ),
        INTF_OP ( edd_describe, struct nvme_device *, nvme_edd_describe ),
};

/** NVMe block interface descriptor */
static struct interface_descriptor nvme_block_desc =
        INTF_DESC ( struct nvme_device, block, nvme_block_op );

static struct nvme_device * nvme_find ( const char *name ) {
    struct nvme_device *nvme;

    /* Look for matching device */
    list_for_each_entry ( nvme, &nvme_devices, list ) {
        return nvme;
        //if ( strcmp ( nvme->func->name, name ) == 0 )
        // 	return usbblk;
    }

    return NULL;
}

/**
 * Open NVMe block device URI
 *
 * @v parent		Parent interface
 * @v uri		URI
 * @ret rc		Return status code
 */
static int nvme_open_uri ( struct interface *parent, struct uri *uri ) {
    struct nvme_device *nvme;
    int rc;
    unsigned long bar_start;
    size_t bar_size;

    /* Sanity check */
    if ( ! uri->opaque )
        return -EINVAL;

    /* Find matching device */
    nvme = nvme_find ( uri->opaque );
    if ( ! nvme )
        return -ENOENT;

    /* Fail if device is already open */
    if ( nvme->opened )
        return -EBUSY;

    ref_init ( &nvme->refcnt, NULL );

    intf_init ( &nvme->block, &nvme_block_desc, &nvme->refcnt );
    intf_plug_plug ( &nvme->block, parent );
    DBGC ( nvme, PCI_FMT " nvme_open_uri(%p) \n", PCI_ARGS ( &nvme->pci_dev ), parent );

    /* Fix up PCI device */
    adjust_pci_device ( &nvme->pci_dev );

    DBGC ( nvme, PCI_FMT " nvme->pci_dev.membase(%p) \n", PCI_ARGS ( &nvme->pci_dev ), nvme->pci_dev.membase );

    // FIXME: HACK! HACK! HACK!
    if (!nvme->pci_dev.membase)
    {
        //pci_write_config_dword( &nvme->pci_dev, PCI_BASE_ADDRESS_0 + 4, 0x1FF00000 );
        //pci_read_config ( &nvme->pci_dev );
    }
    //pci_write_config_dword( &nvme->pci_dev, PCI_BASE_ADDRESS_0 + 4, 0x1FF00000 );
    //pci_write_config_dword( &nvme->pci_dev, PCI_BASE_ADDRESS_0, 0x1FF00000 | PCI_BASE_ADDRESS_MEM_TYPE_MASK );

    /* Map registers */
    bar_start = pci_bar_start ( &nvme->pci_dev, PCI_BASE_ADDRESS_0 );
    bar_size = pci_bar_size ( &nvme->pci_dev, PCI_BASE_ADDRESS_0 );
    struct nvme_reg volatile *reg = (void *) pci_ioremap ( &nvme->pci_dev, bar_start, bar_size );
    if (!reg)
    {
        DBGC ( nvme, PCI_FMT " nvme_open_uri !reg \n", PCI_ARGS ( &nvme->pci_dev ) );
        return -EBUSY;
    }
    nvme->ctrl = malloc(sizeof(*nvme->ctrl));
    nvme->ctrl->reg = reg;
    nvme->ctrl->pci = &nvme->pci_dev;

    uint32_t version = nvme->ctrl->reg->vs;
    DBGC ( nvme, "Found NVMe controller with version %d.%d.%d.\n",
           version >> 16, (version >> 8) & 0xFF, version & 0xFF);
    DBGC ( nvme, "  Capabilities %016llx\n", nvme->ctrl->reg->cap);

    if (~nvme->ctrl->reg->cap & NVME_CAP_CSS_NVME) {
        DBGC ( nvme, "Controller doesn't speak NVMe command set. Skipping.\n");
        return -EBUSY;
    }

    if (nvme_controller_enable(nvme->ctrl)) {
        DBGC ( nvme, "Failed to enable NVMe controller.\n");
        return -EBUSY;
    }

//    for (uint64_t i = 0; i < 2097152; ++i) {
//        DBGC ( nvme, PCI_FMT " reading lba %d()\n", PCI_ARGS ( &nvme->pci_dev ), i );
//
//        int res = nvme_io_xfer(nvme->ctrl->ns, i, virt_to_phys(nvme_dma_buffer), NULL, 1, 0);
//    }

    /* Mark as opened */
    nvme->opened = 1;
    //ref_put ( &nvme->refcnt );

    return 0;
}

/** NVMe device URI opener */
struct uri_opener nvme_uri_opener __uri_opener = {
        .scheme = "nvme",
        .open = nvme_open_uri,
};

/**
 * Probe device
 *
 * @v pci		PCI device
 * @ret rc		Return status code
 */
static int nvme_probe (struct pci_device *pci) {
    struct nvme_device *nvme;
    int rc;

    /* Allocate and initialise structure */
    nvme = zalloc ( sizeof ( *nvme ) );
    if ( ! nvme ) {
        rc = -ENOMEM;
        goto err_alloc;
    }

    nvme->pci_dev = *pci;
    DBGC ( nvme, PCI_FMT " nvme_probe()\n",
           PCI_ARGS ( pci ) );

    process_init_stopped ( &nvme->process, &nvme_process_desc,
                           &nvme->refcnt );

    /* Add to list of devices */
    INIT_LIST_HEAD( &nvme->list );
    list_add_tail ( &nvme->list, &nvme_devices );
    return 0;

    err_alloc:
    return rc;
}

/**
 * Remove device
 *
 * @v func		USB function
 */
static void nvme_remove (struct pci_device *pci) {
    //struct nvme_device *usbblk = usb_func_get_drvdata ( func );

    /* Remove from list of devices */
    //list_del ( &usbblk->list );

    /* Close all interfaces */
    //nvme_scsi_close ( usbblk, -ENODEV );

    /* Shut down interfaces */
    //intfs_shutdown ( -ENODEV, &usbblk->scsi, &usbblk->data, NULL );

    /* Drop reference */
    //ref_put ( &usbblk->refcnt );
}


static struct pci_device_id nvme_pci_ids_dummy[] = {
        PCI_ROM(PCI_ANY_ID, PCI_ANY_ID, "NVMe controller", "NVMe controller", 0),
};

struct pci_driver nvme_driver __pci_driver = {
        .ids = nvme_pci_ids_dummy,
        .id_count = (sizeof(nvme_pci_ids_dummy) / sizeof(nvme_pci_ids_dummy[0])),
        .probe = nvme_probe,
        .remove = nvme_remove,
        .class = PCI_CLASS_ID ( PCI_CLASS_MASSSTORAGE, PCI_CLASS_MASSSTORAGE_NVM,
                                PCI_ANY_ID ),
};
