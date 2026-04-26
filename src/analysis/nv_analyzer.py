# NetVision - nv_analyzer.py
# Network traffic analysis and anomaly detection.
# Developer: MERO:TG@QP4RM

import struct
import socket
import time
import os
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Optional


PROTO_MAP = {
    0: "UNKNOWN",
    1: "TCP",
    2: "UDP",
    3: "HTTP",
    4: "HTTPS",
    5: "DNS",
    6: "ICMP",
    7: "ARP",
    8: "WEBSOCKET",
}

STATUS_MAP = {
    0: "NORMAL",
    1: "SLOW",
    2: "ERROR",
    3: "RETRANSMIT",
    4: "TIMEOUT",
}


@dataclass
class PacketRecord:
    pkt_id: int
    src_ip: str
    dst_ip: str
    src_port: int
    dst_port: int
    protocol: int
    status: int
    size: int
    latency_ms: float
    timestamp: float
    flags: int


@dataclass
class FlowStats:
    src_ip: str
    dst_ip: str
    protocol: int
    packet_count: int = 0
    total_bytes: int = 0
    total_latency: float = 0.0
    max_latency: float = 0.0
    min_latency: float = float("inf")
    error_count: int = 0
    retransmit_count: int = 0
    first_seen: float = 0.0
    last_seen: float = 0.0


@dataclass
class AnomalyReport:
    anomaly_type: str
    severity: str
    src_ip: str
    dst_ip: str
    description: str
    timestamp: float
    metric_value: float


class NetworkAnalyzer:
    def __init__(self):
        self._packets = []
        self._flows = {}
        self._anomalies = []
        self._ip_stats = defaultdict(lambda: {"sent": 0, "recv": 0, "pkts": 0})
        self._latency_threshold = 200.0
        self._pps_threshold = 1000
        self._loss_threshold = 0.05

    def add_packet(self, pkt):
        self._packets.append(pkt)
        self._update_flow(pkt)
        self._update_ip_stats(pkt)
        self._check_anomalies(pkt)

    def _flow_key(self, pkt):
        ips = sorted([pkt.src_ip, pkt.dst_ip])
        return (ips[0], ips[1], pkt.protocol)

    def _update_flow(self, pkt):
        key = self._flow_key(pkt)
        if key not in self._flows:
            self._flows[key] = FlowStats(
                src_ip=key[0],
                dst_ip=key[1],
                protocol=key[2],
                first_seen=pkt.timestamp,
            )
        flow = self._flows[key]
        flow.packet_count += 1
        flow.total_bytes += pkt.size
        flow.total_latency += pkt.latency_ms
        flow.last_seen = pkt.timestamp

        if pkt.latency_ms > flow.max_latency:
            flow.max_latency = pkt.latency_ms
        if pkt.latency_ms < flow.min_latency:
            flow.min_latency = pkt.latency_ms
        if pkt.status == 2:
            flow.error_count += 1
        if pkt.status == 3:
            flow.retransmit_count += 1

    def _update_ip_stats(self, pkt):
        self._ip_stats[pkt.src_ip]["sent"] += pkt.size
        self._ip_stats[pkt.src_ip]["pkts"] += 1
        self._ip_stats[pkt.dst_ip]["recv"] += pkt.size

    def _check_anomalies(self, pkt):
        if pkt.latency_ms > self._latency_threshold:
            self._anomalies.append(AnomalyReport(
                anomaly_type="HIGH_LATENCY",
                severity="WARNING",
                src_ip=pkt.src_ip,
                dst_ip=pkt.dst_ip,
                description="Latency exceeded {:.1f}ms threshold: {:.1f}ms".format(
                    self._latency_threshold, pkt.latency_ms),
                timestamp=pkt.timestamp,
                metric_value=pkt.latency_ms,
            ))

        if pkt.status == 2:
            self._anomalies.append(AnomalyReport(
                anomaly_type="PACKET_ERROR",
                severity="ERROR",
                src_ip=pkt.src_ip,
                dst_ip=pkt.dst_ip,
                description="Packet error detected",
                timestamp=pkt.timestamp,
                metric_value=0.0,
            ))

        key = self._flow_key(pkt)
        flow = self._flows.get(key)
        if flow and flow.packet_count > 100:
            loss = flow.retransmit_count / flow.packet_count
            if loss > self._loss_threshold:
                self._anomalies.append(AnomalyReport(
                    anomaly_type="PACKET_LOSS",
                    severity="CRITICAL",
                    src_ip=pkt.src_ip,
                    dst_ip=pkt.dst_ip,
                    description="Packet loss rate {:.2%} exceeds threshold".format(loss),
                    timestamp=pkt.timestamp,
                    metric_value=loss,
                ))

    def detect_ddos(self, window_sec=10.0):
        results = []
        if not self._packets:
            return results

        now = self._packets[-1].timestamp
        window_start = now - window_sec
        recent = [p for p in self._packets if p.timestamp >= window_start]

        dst_counts = defaultdict(int)
        for p in recent:
            dst_counts[p.dst_ip] += 1

        for ip, count in dst_counts.items():
            pps = count / window_sec
            if pps > self._pps_threshold:
                results.append(AnomalyReport(
                    anomaly_type="DDOS_SUSPECTED",
                    severity="CRITICAL",
                    src_ip="multiple",
                    dst_ip=ip,
                    description="Target receiving {:.0f} packets/sec from multiple sources".format(pps),
                    timestamp=now,
                    metric_value=pps,
                ))
        return results

    def get_top_talkers(self, n=10):
        sorted_ips = sorted(
            self._ip_stats.items(),
            key=lambda x: x[1]["sent"] + x[1]["recv"],
            reverse=True,
        )
        return sorted_ips[:n]

    def get_flow_summary(self):
        result = []
        for key, flow in sorted(self._flows.items(), key=lambda x: x[1].total_bytes, reverse=True):
            avg_latency = flow.total_latency / flow.packet_count if flow.packet_count > 0 else 0
            result.append({
                "src": flow.src_ip,
                "dst": flow.dst_ip,
                "protocol": PROTO_MAP.get(flow.protocol, "UNKNOWN"),
                "packets": flow.packet_count,
                "bytes": flow.total_bytes,
                "avg_latency_ms": round(avg_latency, 2),
                "max_latency_ms": round(flow.max_latency, 2),
                "errors": flow.error_count,
                "retransmits": flow.retransmit_count,
                "duration_sec": round(flow.last_seen - flow.first_seen, 2),
            })
        return result

    def get_anomalies(self, severity=None):
        if severity:
            return [a for a in self._anomalies if a.severity == severity]
        return list(self._anomalies)

    def get_protocol_distribution(self):
        dist = defaultdict(int)
        for p in self._packets:
            name = PROTO_MAP.get(p.protocol, "UNKNOWN")
            dist[name] += 1
        total = len(self._packets)
        return {k: {"count": v, "percent": round(v / total * 100, 2) if total > 0 else 0}
                for k, v in sorted(dist.items(), key=lambda x: x[1], reverse=True)}

    def get_bandwidth_timeline(self, interval_sec=1.0):
        if not self._packets:
            return []

        start = self._packets[0].timestamp
        end = self._packets[-1].timestamp
        timeline = []
        t = start

        while t < end:
            window = [p for p in self._packets if t <= p.timestamp < t + interval_sec]
            total_bytes = sum(p.size for p in window)
            timeline.append({
                "time": round(t - start, 2),
                "bytes_per_sec": total_bytes / interval_sec,
                "packets_per_sec": len(window) / interval_sec,
            })
            t += interval_sec

        return timeline

    def export_report(self, filepath):
        report = []
        report.append("=" * 60)
        report.append("NetVision Analysis Report")
        report.append("Developer: MERO:TG@QP4RM")
        report.append("Generated: {}".format(time.strftime("%Y-%m-%d %H:%M:%S")))
        report.append("=" * 60)
        report.append("")

        report.append("SUMMARY")
        report.append("-" * 40)
        report.append("Total Packets:  {}".format(len(self._packets)))
        report.append("Total Flows:    {}".format(len(self._flows)))
        report.append("Unique IPs:     {}".format(len(self._ip_stats)))
        report.append("Anomalies:      {}".format(len(self._anomalies)))
        report.append("")

        report.append("PROTOCOL DISTRIBUTION")
        report.append("-" * 40)
        for proto, info in self.get_protocol_distribution().items():
            report.append("  {:12s}  {:>6d} packets  ({:.1f}%)".format(
                proto, info["count"], info["percent"]))
        report.append("")

        report.append("TOP TALKERS")
        report.append("-" * 40)
        for ip, stats in self.get_top_talkers(10):
            report.append("  {:20s}  Sent: {:>10d} B  Recv: {:>10d} B  Pkts: {:>6d}".format(
                ip, stats["sent"], stats["recv"], stats["pkts"]))
        report.append("")

        report.append("FLOW DETAILS")
        report.append("-" * 40)
        for flow in self.get_flow_summary()[:20]:
            report.append("  {} -> {} [{}]".format(flow["src"], flow["dst"], flow["protocol"]))
            report.append("    Packets: {}  Bytes: {}  AvgLatency: {}ms  Errors: {}".format(
                flow["packets"], flow["bytes"], flow["avg_latency_ms"], flow["errors"]))
        report.append("")

        anomalies = self.get_anomalies()
        if anomalies:
            report.append("ANOMALIES")
            report.append("-" * 40)
            for a in anomalies[-50:]:
                report.append("  [{}] {} | {} -> {} | {}".format(
                    a.severity, a.anomaly_type, a.src_ip, a.dst_ip, a.description))

        text = "\n".join(report)
        with open(filepath, "w") as f:
            f.write(text)
        return text

    def set_thresholds(self, latency_ms=None, pps=None, loss_rate=None):
        if latency_ms is not None:
            self._latency_threshold = latency_ms
        if pps is not None:
            self._pps_threshold = pps
        if loss_rate is not None:
            self._loss_threshold = loss_rate

    def clear(self):
        self._packets.clear()
        self._flows.clear()
        self._anomalies.clear()
        self._ip_stats.clear()
