import argparse
import math
import struct
import time
import subprocess


def calculate_sine_val(index, min_val=40, max_val=400, period=20):
    """Calculates a single sine wave integer value for a given index."""
    midpoint = (max_val + min_val) / 2.0
    amplitude = (max_val - min_val) / 2.0
    return round(midpoint + amplitude * math.sin(2 * math.pi * index / period))


def pack_bgl_delta(
    delta_value,
    expired=0,
    hidden=0,
    undefined=0,
    display_units=0,
    is_mmol=0,
):
    """Packs the comm_bgl_delta_t union into a uint16 integer/bytes.

    Bit layout for Byte 1:
    Bit 0-2 : Padding (3 bits)
    Bit 3   : expired (1 bit)
    Bit 4   : hidden (1 bit)
    Bit 5   : undefined (1 bit)
    Bit 6   : display_units (1 bit)
    Bit 7   : is_mmol (1 bit)
    """
    # Restrict signed delta to int8 bounds (-128 to 127)
    delta_clamped = max(-128, min(127, delta_value))

    # Construct status byte via bitwise shifts
    flags_byte = (
        ((expired & 0x01) << 3)
        | ((hidden & 0x01) << 4)
        | ((undefined & 0x01) << 5)
        | ((display_units & 0x01) << 6)
        | ((is_mmol & 0x01) << 7)
    )

    # Pack as <bB (signed char, unsigned char) -> 16-bit integer
    raw_bytes = struct.pack("<bB", delta_clamped, flags_byte)

    # Unpack raw 16-bit integer representation
    (raw_int16,) = struct.unpack("<H", raw_bytes)

    return raw_int16, raw_bytes.hex()


def generate_initial_series(
    num_points=16,
    interval_minutes=5,
    min_val=40,
    max_val=400,
    period=20,
    mmol=False,
):
    """Generates the initial bulk payload (index 2009) for the starting window."""
    last_timestamp = int(time.time())

    raw_values = [
        calculate_sine_val(i, min_val, max_val, period)
        for i in range(num_points)
    ]

    formatted_values = []
    for val in raw_values:
        sine_val = val
        if mmol:
            sine_val |= 0x8000
        formatted_values.append(sine_val)

    fmt = f"<IH{num_points}H"
    binary_data = struct.pack(
        fmt, last_timestamp, num_points, *formatted_values
    )
    hex_string = binary_data.hex()

    return hex_string, raw_values, last_timestamp


def main():
    parser = argparse.ArgumentParser(
        description="Generate and continuously send Pebble sine wave data."
    )

    parser.add_argument(
        "-n",
        "--num-points",
        type=int,
        default=16,
        help="Initial number of data points (default: 16)",
    )
    parser.add_argument(
        "-i",
        "--interval",
        type=int,
        default=5,
        help="Interval in minutes between points (default: 5)",
    )
    parser.add_argument(
        "-m",
        "--mmol",
        action="store_true",
        help="Set MSB/flags indicating mmol/L instead of mg/dL",
    )
    parser.add_argument(
        "-u",
        "--deltau",
        action="store_true",
        help="Set flag to display delta units",
    )

    args = parser.parse_args()
    interval_seconds = args.interval * 60

    # 1. Generate and send initial series for index 2009
    hex_output, initial_raw_values, last_timestamp = generate_initial_series(
        num_points=args.num_points,
        interval_minutes=args.interval,
        mmol=args.mmol,
    )

    # Pack comm_bgl_delta_t struct for index 2001
    is_mmol_flag = 1 if args.mmol else 0
    is_deltau_flag = 1 if args.deltau else 0
    delta=0
    delta_raw_int, delta_hex = pack_bgl_delta(
        delta_value=delta,
	undefined=1,
        display_units=is_deltau_flag,
        is_mmol=is_mmol_flag,
    )
    print(f"Sending initial series (2009={hex_output})...")
    subprocess.run(
        ["pebble", "send-app-message", "--bytes", f"2001={delta_hex}",f"2009={hex_output}"]
    )

    # Track state for loop iterations
    current_step = args.num_points - 1
    prev_raw_value = initial_raw_values[-1]

    # 2. Continuous loop
    while True:
        print(f"\nWaiting {args.interval} minutes for next reading...")
        time.sleep(interval_seconds)

        current_step += 1
        current_time = int(time.time())

        # Calculate new point and raw numerical delta
        curr_raw_value = calculate_sine_val(current_step)
        delta = curr_raw_value - prev_raw_value

        # Format current value with optional mmol flag
        curr_formatted_value = curr_raw_value
        if args.mmol:
            curr_formatted_value |= 0x8000

        # Pack timestamped value for index 2002
        packed_2002 = struct.pack(
            "<IH", current_time, curr_formatted_value
        ).hex()

        # Pack comm_bgl_delta_t struct for index 2001
        delta_raw_int, delta_hex = pack_bgl_delta(
            delta_value=delta,
	    undefined=0,
            display_units=is_deltau_flag,
            is_mmol=is_mmol_flag,
        )

        print(
            f"[Step {current_step}] Time: {current_time} | Value: {curr_raw_value} | Delta: {delta}"
        )
        print(
            f"Sending 2002 (--bytes) = {packed_2002} | 2001 (--bytes) = {delta} as {delta_hex}"
        )

        # Send index 2002 (--bytes timestamped value)
        subprocess.run(
            ["pebble", "send-app-message", "--bytes", f"2002={packed_2002}"]
        )

        # Send index 2001 as --bytes (hex representation)
        subprocess.run(
            ["pebble", "send-app-message", "--bytes", f"2001={delta_hex}"]
        )

        prev_raw_value = curr_raw_value


if __name__ == "__main__":
    main()
