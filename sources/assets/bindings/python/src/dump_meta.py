import sys
import json
from hako_parser.parse_meta import HakoMetaParser

def main():
    args = sys.argv[1:]
    if len(args) == 0:
        print("Usage: python dump_meta.py <MasterData> [output.json]")
        return

    input_path = args[0]
    output_path = args[1] if len(args) > 1 else None

    with open(input_path, "rb") as f:
        data = bytearray(f.read())

    parser = HakoMetaParser()
    parser.parse(data)
    meta_info = parser.get_meta_info()
    if not meta_info:
        print("Failed to parse meta info.")
        return
    if output_path:
        with open(output_path, "w") as f:
            json.dump(meta_info, f, indent=2)
    else:
        print(json.dumps(meta_info, indent=2))

if __name__ == "__main__":
    main()
