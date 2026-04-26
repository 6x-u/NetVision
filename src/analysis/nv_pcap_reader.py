# NetVision - nv_pcap_reader.py
# PCAP file reader for offline analysis.
# Developer: MERO:TG@QP4RM

import struct
import socket
from .nv_analyzer import PacketRecord

PCAP_MAGIC = 0xA1B2C3D4
PCAP_MAGIC_NANO = 0xA1B23C4D
LINKTYPE_ETHERNET = 1

ETHERTYPE_IP = 0x0800
ETHERTYPE_ARP = 0x0806

IPPROTO_TCP = 6
IPPROTO_UDP = 17
IPPROTO_ICMP = 1


class PcapReader:
    def __init__(self, filepath):
        self._filepath = filepath
        self._packets = []
        self._nano = False
        self._linktype = 0

    def read(self):
        with open(self._filepath, "rb") as f:
            self._read_header(f)
            pkt_id = 1
            while True:
                pkt = self._read_packet(f, pkt_id)
                if pkt is None:
                    break
                self._packets.append(pkt)
                pkt_id += 1
        return self._packets

    def _read_header(self, f):
        data = f.read(24)
        if len(data) < 24:
            raise ValueError("Invalid pcap file: header too short")

        magic = struct.unpack("<I", data[0:4])[0]
        if magic == PCAP_MAGIC:
            self._nano = False
        elif magic == PCAP_MAGIC_NANO:
            self._nano = True
        else:
            raise ValueError("Invalid pcap magic: 0x{:08X}".format(magic))

        self._linktype = struct.unpack("<I", data[20:24])[0]

    def _read_packet(self, f, pkt_id):
        header = f.read(16)
        if len(header) < 16:
            return None

        ts_sec, ts_usec, caplen, origlen = struct.unpack("<IIII", header)
        if self._nano:
            timestamp = ts_sec + ts_usec / 1e9
        else:
            timestamp = ts_sec + ts_usec / 1e6

        raw = f.read(caplen)
        if len(raw) < caplen:
            return None

        if self._linktype != LINKTYPE_ETHERNET:
            return PacketRecord(
                pkt_id=pkt_id, src_ip="0.0.0.0", dst_ip="0.0.0.0",
                src_port=0, dst_port=0, protocol=0, status=0,
                size=origlen, latency_ms=0.0, timestamp=timestamp, flags=0,
            )

        return self._parse_ethernet(raw, pkt_id, origlen, timestamp)

    def _parse_ethernet(self, raw, pkt_id, origlen, timestamp):
        if len(raw) < 14:
            return None

        ethertype = struct.unpack("!H", raw[12:14])[0]

        if ethertype == ETHERTYPE_ARP:
            return PacketRecord(
                pkt_id=pkt_id, src_ip="0.0.0.0", dst_ip="0.0.0.0",
                src_port=0, dst_port=0, protocol=7, status=0,
                size=origlen, latency_ms=0.0, timestamp=timestamp, flags=0,
            )

        if ethertype != ETHERTYPE_IP:
            return None

        return self._parse_ip(raw[14:], pkt_id, origlen, timestamp)

    def _parse_ip(self, data, pkt_id, origlen, timestamp):
        if len(data) < 20:
            return None

        ihl = (data[0] & 0x0F) * 4
        protocol = data[9]
        src_ip = socket.inet_ntoa(data[12:16])
        dst_ip = socket.inet_ntoa(data[16:20])

        src_port = 0
        dst_port = 0
        flags = 0
        nv_proto = 0

        if protocol == IPPROTO_TCP and len(data) >= ihl + 20:
            src_port, dst_port = struct.unpack("!HH", data[ihl:ihl + 4])
            flags = data[ihl + 13]
            nv_proto = self._detect_proto_tcp(src_port, dst_port)

        elif protocol == IPPROTO_UDP and len(data) >= ihl + 8:
            src_port, dst_port = struct.unpack("!HH", data[ihl:ihl + 4])
            if src_port == 53 or dst_port == 53:
                nv_proto = 5
            else:
                nv_proto = 2

        elif protocol == IPPROTO_ICMP:
            nv_proto = 6

        return PacketRecord(
            pkt_id=pkt_id,
            src_ip=src_ip,
            dst_ip=dst_ip,
            src_port=src_port,
            dst_port=dst_port,
            protocol=nv_proto,
            status=0,
            size=origlen,
            latency_ms=0.0,
            timestamp=timestamp,
            flags=flags,
        )

    def _detect_proto_tcp(self, src_port, dst_port):
        if src_port == 80 or dst_port == 80:
            return 3
        if src_port == 443 or dst_port == 443:
            return 4
        if src_port == 8080 or dst_port == 8080:
            return 3
        return 1

    def get_packets(self):
        return list(self._packets)

    def packet_count(self):
        return len(self._packets)
