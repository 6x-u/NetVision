# NetVision - nv_cli.py
# Command-line analysis tool.
# Developer: MERO:TG@QP4RM

import sys
import argparse
from .nv_analyzer import NetworkAnalyzer
from .nv_pcap_reader import PcapReader


def run_analysis(pcap_file, output_file=None, top_n=10, latency_threshold=200.0):
    reader = PcapReader(pcap_file)
    packets = reader.read()

    analyzer = NetworkAnalyzer()
    analyzer.set_thresholds(latency_ms=latency_threshold)

    for pkt in packets:
        analyzer.add_packet(pkt)

    print("=" * 60)
    print("NetVision - Network Analysis")
    print("Developer: MERO:TG@QP4RM")
    print("=" * 60)
    print()
    print("File: {}".format(pcap_file))
    print("Packets: {}".format(len(packets)))
    print()

    print("Protocol Distribution:")
    for proto, info in analyzer.get_protocol_distribution().items():
        print("  {:12s}  {:>6d} ({:.1f}%)".format(proto, info["count"], info["percent"]))
    print()

    print("Top {} Talkers:".format(top_n))
    for ip, stats in analyzer.get_top_talkers(top_n):
        print("  {:20s}  Sent: {:>10d} B  Recv: {:>10d} B".format(
            ip, stats["sent"], stats["recv"]))
    print()

    anomalies = analyzer.get_anomalies()
    if anomalies:
        print("Anomalies Detected: {}".format(len(anomalies)))
        for a in anomalies[-20:]:
            print("  [{}] {} -> {}: {}".format(a.severity, a.src_ip, a.dst_ip, a.description))
    else:
        print("No anomalies detected.")
    print()

    ddos = analyzer.detect_ddos()
    if ddos:
        print("DDoS Alerts:")
        for d in ddos:
            print("  {} -> {}: {}".format(d.src_ip, d.dst_ip, d.description))
    print()

    if output_file:
        analyzer.export_report(output_file)
        print("Report saved to: {}".format(output_file))

    return analyzer


def main():
    parser = argparse.ArgumentParser(
        prog="netvision-analyze",
        description="NetVision Network Traffic Analyzer - MERO:TG@QP4RM",
    )
    parser.add_argument("pcap", help="Path to pcap file")
    parser.add_argument("-o", "--output", help="Output report file path")
    parser.add_argument("-n", "--top", type=int, default=10, help="Number of top talkers")
    parser.add_argument("-l", "--latency", type=float, default=200.0, help="Latency threshold (ms)")

    args = parser.parse_args()
    run_analysis(args.pcap, args.output, args.top, args.latency)


if __name__ == "__main__":
    main()
