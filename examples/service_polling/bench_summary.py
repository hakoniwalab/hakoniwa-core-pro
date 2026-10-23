"""Summarize bench.bash results: round trip and its parts per image size."""
import csv
import statistics
import sys
from pathlib import Path


def load(path):
    with open(path) as f:
        return {int(r["seq"]): r for r in csv.DictReader(f)}


def stats(values):
    values = sorted(values)
    p99 = values[min(len(values) - 1, int(len(values) * 0.99))]
    return statistics.mean(values), p99, values[-1]


def main():
    work = Path(sys.argv[1])
    print("size,bytes,n,rtt_mean_ms,rtt_p99_ms,rtt_max_ms,"
          "to_server_ms,fill_ms,encode_ms,put_ms,to_client_ms,decode_ms,MB_per_s,frames_per_s")
    for size in sys.argv[2:]:
        client = load(work / size / "client.csv")
        server = load(work / size / "server.csv")
        rows = [(c, server[s]) for s, c in client.items() if s in server]
        if not rows:
            print(f"{size},no data")
            continue
        ms = lambda a, b: [(int(r[b]) - int(r[a])) / 1e6 for r in a_rows] if False else None
        rtt = [(int(c["response_in_ns"]) - int(c["send_ns"])) / 1e6 for c, _ in rows]
        part = lambda a_of, a, b_of, b: statistics.mean(
            (int(rb[b]) - int(ra[a])) / 1e6
            for c, s in rows
            for ra, rb in [((c if a_of == "c" else s), (c if b_of == "c" else s))])
        to_server = part("c", "send_ns", "s", "detect_ns")
        fill = part("s", "request_read_ns", "s", "image_filled_ns")
        encode = part("s", "image_filled_ns", "s", "encoded_ns")
        put = part("s", "encoded_ns", "s", "put_ns")
        to_client = part("s", "put_ns", "c", "response_in_ns")
        decode = part("c", "response_in_ns", "c", "decoded_ns")
        nbytes = int(rows[0][0]["bytes"])
        mean, p99, mx = stats(rtt)
        mbps = nbytes / (mean / 1e3) / 1e6 if nbytes else 0.0
        fps = 1e3 / (mean + decode)
        print(f"{size},{nbytes},{len(rows)},{mean:.3f},{p99:.3f},{mx:.3f},"
              f"{to_server:.3f},{fill:.3f},{encode:.3f},{put:.3f},{to_client:.3f},{decode:.3f},"
              f"{mbps:.0f},{fps:.0f}")


if __name__ == "__main__":
    main()
