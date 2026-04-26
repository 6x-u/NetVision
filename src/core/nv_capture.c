/*
 * NetVision - nv_capture.c
 * Developer: MERO:TG@QP4RM
 */

#include "nv_capture.h"
#include <pcap/pcap.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <sys/time.h>
#include <pthread.h>

struct nv_capture_ctx {
    pcap_t           *handle;
    char              device[NV_MAX_NAME_LEN];
    char              errbuf[PCAP_ERRBUF_SIZE];
    int               snaplen;
    int               promisc;
    int               running;
    pthread_t         thread;
    pthread_mutex_t   mutex;
    nv_ring_buffer_t  ring;
    uint32_t          pkt_id_counter;
};

static double nv_get_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static nv_protocol_t nv_detect_protocol(uint8_t ip_proto, uint16_t src_port, uint16_t dst_port)
{
    if (ip_proto == IPPROTO_ICMP)
        return NV_PROTO_ICMP;

    if (ip_proto == IPPROTO_UDP) {
        if (src_port == 53 || dst_port == 53)
            return NV_PROTO_DNS;
        return NV_PROTO_UDP;
    }

    if (ip_proto == IPPROTO_TCP) {
        if (src_port == 80 || dst_port == 80)
            return NV_PROTO_HTTP;
        if (src_port == 443 || dst_port == 443)
            return NV_PROTO_HTTPS;
        if (src_port == 8080 || dst_port == 8080)
            return NV_PROTO_HTTP;
        return NV_PROTO_TCP;
    }

    return NV_PROTO_UNKNOWN;
}

static int nv_ring_init(nv_ring_buffer_t *rb, uint32_t capacity)
{
    rb->buffer = (nv_packet_t *)calloc(capacity, sizeof(nv_packet_t));
    if (!rb->buffer)
        return -1;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    return 0;
}

static void nv_ring_free(nv_ring_buffer_t *rb)
{
    if (rb->buffer) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
}

static int nv_ring_push(nv_ring_buffer_t *rb, const nv_packet_t *pkt)
{
    if (!rb->buffer)
        return -1;
    rb->buffer[rb->head] = *pkt;
    rb->head = (rb->head + 1) % rb->capacity;
    if (rb->count < rb->capacity)
        rb->count++;
    else
        rb->tail = (rb->tail + 1) % rb->capacity;
    return 0;
}

static int nv_ring_pop(nv_ring_buffer_t *rb, nv_packet_t *pkt)
{
    if (!rb->buffer || rb->count == 0)
        return -1;
    *pkt = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    return 0;
}

static void nv_capture_callback(u_char *user, const struct pcap_pkthdr *hdr, const u_char *data)
{
    nv_capture_ctx_t *ctx = (nv_capture_ctx_t *)user;
    nv_packet_t pkt;
    const struct ether_header *eth;
    const struct ip *ip_hdr;
    const struct tcphdr *tcp_hdr;
    const struct udphdr *udp_hdr;
    uint16_t src_port = 0, dst_port = 0;

    if (hdr->caplen < sizeof(struct ether_header))
        return;

    memset(&pkt, 0, sizeof(pkt));
    eth = (const struct ether_header *)data;

    if (ntohs(eth->ether_type) == ETHERTYPE_ARP) {
        pkt.protocol = NV_PROTO_ARP;
        pkt.size = hdr->len;
        pkt.timestamp = nv_get_timestamp();
        pkt.status = NV_PKT_NORMAL;
        pkt.visible = 1;
        pkt.anim_progress = 0.0f;
        pthread_mutex_lock(&ctx->mutex);
        pkt.id = ctx->pkt_id_counter++;
        nv_ring_push(&ctx->ring, &pkt);
        pthread_mutex_unlock(&ctx->mutex);
        return;
    }

    if (ntohs(eth->ether_type) != ETHERTYPE_IP)
        return;

    if (hdr->caplen < sizeof(struct ether_header) + sizeof(struct ip))
        return;

    ip_hdr = (const struct ip *)(data + sizeof(struct ether_header));
    pkt.size = hdr->len;
    pkt.timestamp = nv_get_timestamp();
    pkt.status = NV_PKT_NORMAL;
    pkt.visible = 1;
    pkt.anim_progress = 0.0f;

    pkt.src_node_id = ntohl(ip_hdr->ip_src.s_addr);
    pkt.dst_node_id = ntohl(ip_hdr->ip_dst.s_addr);

    if (ip_hdr->ip_p == IPPROTO_TCP) {
        if (hdr->caplen >= sizeof(struct ether_header) + (ip_hdr->ip_hl * 4) + sizeof(struct tcphdr)) {
            tcp_hdr = (const struct tcphdr *)((const u_char *)ip_hdr + ip_hdr->ip_hl * 4);
            src_port = ntohs(tcp_hdr->th_sport);
            dst_port = ntohs(tcp_hdr->th_dport);
            pkt.flags = tcp_hdr->th_flags;
        }
    } else if (ip_hdr->ip_p == IPPROTO_UDP) {
        if (hdr->caplen >= sizeof(struct ether_header) + (ip_hdr->ip_hl * 4) + sizeof(struct udphdr)) {
            udp_hdr = (const struct udphdr *)((const u_char *)ip_hdr + ip_hdr->ip_hl * 4);
            src_port = ntohs(udp_hdr->uh_sport);
            dst_port = ntohs(udp_hdr->uh_dport);
        }
    }

    pkt.src_port = src_port;
    pkt.dst_port = dst_port;
    pkt.protocol = nv_detect_protocol(ip_hdr->ip_p, src_port, dst_port);

    pthread_mutex_lock(&ctx->mutex);
    pkt.id = ctx->pkt_id_counter++;
    nv_ring_push(&ctx->ring, &pkt);
    pthread_mutex_unlock(&ctx->mutex);
}

static void *nv_capture_thread(void *arg)
{
    nv_capture_ctx_t *ctx = (nv_capture_ctx_t *)arg;
    while (ctx->running) {
        int ret = pcap_dispatch(ctx->handle, 64, nv_capture_callback, (u_char *)ctx);
        if (ret < 0)
            break;
    }
    return NULL;
}

NV_API nv_capture_ctx_t *nv_capture_create(const char *device, int snaplen, int promisc)
{
    nv_capture_ctx_t *ctx = (nv_capture_ctx_t *)calloc(1, sizeof(nv_capture_ctx_t));
    if (!ctx)
        return NULL;

    if (device)
        strncpy(ctx->device, device, NV_MAX_NAME_LEN - 1);
    ctx->snaplen = (snaplen > 0) ? snaplen : 65535;
    ctx->promisc = promisc;
    ctx->running = 0;
    ctx->pkt_id_counter = 1;

    if (nv_ring_init(&ctx->ring, NV_RING_BUFFER_SIZE) != 0) {
        free(ctx);
        return NULL;
    }

    pthread_mutex_init(&ctx->mutex, NULL);
    return ctx;
}

NV_API void nv_capture_destroy(nv_capture_ctx_t *ctx)
{
    if (!ctx)
        return;
    if (ctx->running)
        nv_capture_stop(ctx);
    nv_ring_free(&ctx->ring);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
}

NV_API int nv_capture_start(nv_capture_ctx_t *ctx)
{
    if (!ctx || ctx->running)
        return -1;

    ctx->handle = pcap_open_live(
        ctx->device[0] ? ctx->device : NULL,
        ctx->snaplen,
        ctx->promisc,
        100,
        ctx->errbuf
    );

    if (!ctx->handle)
        return -1;

    ctx->running = 1;
    if (pthread_create(&ctx->thread, NULL, nv_capture_thread, ctx) != 0) {
        pcap_close(ctx->handle);
        ctx->handle = NULL;
        ctx->running = 0;
        return -1;
    }

    return 0;
}

NV_API int nv_capture_stop(nv_capture_ctx_t *ctx)
{
    if (!ctx || !ctx->running)
        return -1;

    ctx->running = 0;
    if (ctx->handle)
        pcap_breakloop(ctx->handle);
    pthread_join(ctx->thread, NULL);

    if (ctx->handle) {
        pcap_close(ctx->handle);
        ctx->handle = NULL;
    }
    return 0;
}

NV_API int nv_capture_set_filter(nv_capture_ctx_t *ctx, const char *filter_expr)
{
    struct bpf_program fp;

    if (!ctx || !ctx->handle || !filter_expr)
        return -1;

    if (pcap_compile(ctx->handle, &fp, filter_expr, 0, PCAP_NETMASK_UNKNOWN) != 0)
        return -1;

    if (pcap_setfilter(ctx->handle, &fp) != 0) {
        pcap_freecode(&fp);
        return -1;
    }

    pcap_freecode(&fp);
    return 0;
}

NV_API int nv_capture_next_packet(nv_capture_ctx_t *ctx, nv_packet_t *pkt)
{
    int ret;
    if (!ctx || !pkt)
        return -1;
    pthread_mutex_lock(&ctx->mutex);
    ret = nv_ring_pop(&ctx->ring, pkt);
    pthread_mutex_unlock(&ctx->mutex);
    return ret;
}

NV_API int nv_capture_is_running(nv_capture_ctx_t *ctx)
{
    return ctx ? ctx->running : 0;
}

NV_API const char *nv_capture_get_error(nv_capture_ctx_t *ctx)
{
    return ctx ? ctx->errbuf : "null context";
}

NV_API int nv_capture_list_devices(char devices[][NV_MAX_NAME_LEN], int max_devices)
{
    pcap_if_t *alldevs, *dev;
    char errbuf[PCAP_ERRBUF_SIZE];
    int count = 0;

    if (pcap_findalldevs(&alldevs, errbuf) != 0)
        return -1;

    for (dev = alldevs; dev && count < max_devices; dev = dev->next) {
        strncpy(devices[count], dev->name, NV_MAX_NAME_LEN - 1);
        devices[count][NV_MAX_NAME_LEN - 1] = '\0';
        count++;
    }

    pcap_freealldevs(alldevs);
    return count;
}
