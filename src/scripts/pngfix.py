import struct
import sys

def remove_phys_chunk(input_path, output_path):
    with open(input_path, "rb") as f:
        data = f.read()

    signature = data[:8]
    if signature != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Not a valid PNG file")

    output = bytearray(signature)
    pos = 8
    removed = []

    while pos < len(data):
        # Each chunk: 4-byte length, 4-byte type, data, 4-byte CRC
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        chunk_type = data[pos + 4:pos + 8]
        chunk_total_len = 4 + 4 + length + 4  # length + type + data + crc

        if chunk_type not in [b"IHDR", b"IDAT", b"PLTE", b"IEND", b"tRNS", b"acTL", b"adTL", b"fdAT", b"fcTL"]:
            removed.append(chunk_type)
            pos += chunk_total_len
            continue  # skip writing this chunk

        output += data[pos:pos + chunk_total_len]
        pos += chunk_total_len

    with open(output_path, "wb") as f:
        f.write(output)

    if len(removed) > 0:
        for i in removed:
            print("Removed: %s chunk" % (i.decode()))
    else:
        print("pHYs chunk removed.")


if len(sys.argv) < 3:
    print("Need input and output file")
    exit(1)

inp = sys.argv[1]
outp = sys.argv[2]

# Example usage:
remove_phys_chunk(inp, outp)
