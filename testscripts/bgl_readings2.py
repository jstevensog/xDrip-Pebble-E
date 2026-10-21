import argparse
import json
import math
import os
import struct
import subprocess
import time

STATE_FILE = "bgl_state.json"


def calculate_sine_val(index, min_val=40, max_val=400, period=20):
    """Calculates a single sine wave integer value for a given index."""
    midpoint = (max_val + min_val) / 2.0
    amplitude = (max_val - min_val) / 2.0
    return round(midpoint + amplitude * math.sin(2 * math.pi * index / period))


def slope_ordinal(bg_slope_value):
    """Calculate the slope value"""
    print(f"bg_slope_value = {bg_slope_value}")
    match bg_slope_value:
        case x if x <= -63:
            return 7
        case x if x <= -37:
            return 6
        case x if x <= -18:
            return 5
        case x if x <= 18:
            return 4
        case x if x <= 37:
            return 3
        case x if x <= 63:
            return 2
        case _:
            return 1


def pack_bgl_delta(
    delta_value,
    expired=0,
    hidden=0,
    undefined=0,
    display_units=0,
    is_mmol=0,
):
    """Packs the comm_bgl_delta_t union into a uint16 integer/bytes."""
    delta_clamped = max(-128, min(127, delta_value))

    flags_byte = (
        ((expired & 0x01) << 3)
        | ((hidden & 0x01) << 4)
        | ((undefined & 0x01) << 5)
        | ((display_units & 0x01) << 6)
        | ((is_mmol & 0x01) << 7)
    )

    raw_bytes = struct.pack("<bB", delta_clamped, flags_byte)
    (raw_int16,) = struct.unpack("<H", raw_bytes)

    return raw_int16, raw_bytes.hex()


def generate_series_payload(
    start_step,
    num_points,
    min_val=40,
    max_val=400,
    period=20,
    mmol=False,
):
    """Generates a bulk payload (index 2009) starting from start_step for num_points."""
    last_timestamp = int(time.time())

    raw_values = [
        calculate_sine_val(start_step + i, min_val, max_val, period)
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


def load_state():
    """Loads state from file if it exists."""
    if os.path.exists(STATE_FILE):
        try:
            with open(STATE_FILE, "r") as f:
                state = json.load(f)
                print(f"Loaded existing state from {STATE_FILE}: {state}")
                return state
        except Exception as e:
            print(f"Failed to load state file: {e}. Starting fresh.")
    return None


def save_state(current_step, prev_raw_value, last_timestamp):
    """Saves state variables to JSON file."""
    state = {
        "current_step": current_step,
        "prev_raw_value": prev_raw_value,
        "last_timestamp": last_timestamp,
    }
    with open(STATE_FILE, "w") as f:
        json.dump(state, f, indent=2)


def main():
    parser = argparse.ArgumentParser(
        description="Generate and continuously send Pebble sine wave data."
    )

    parser.add_argument(
        "-n",
        "--num-points",
        type=int,
        default=16,
        help="Initial/max number of data points for bulk load (default: 16)",
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
    parser.add_argument(
        "-s",
        "--send_slope",
        action="store_true",
        help="Send the slope value with the BGL",
    )
    parser.add_argument(
        "-r",
        "--reset",
        action="store_true",
        help="Reset state and force initial series load",
    )

    args = parser.parse_args()
    interval_seconds = args.interval * 60
    is_mmol_flag = 1 if args.mmol else 0
    is_deltau_flag = 1 if args.deltau else 0

    state = None if args.reset else load_state()

    if state is None:
        # 1. Initial run or reset: start series from index 0
        slope_val = 0
        hex_output, initial_raw_values, last_timestamp = generate_series_payload(
            start_step=0,
            num_points=args.num_points,
            mmol=args.mmol,
        )

        delta = 0
        delta_raw_int, delta_hex = pack_bgl_delta(
            delta_value=delta,
            undefined=1,
            display_units=is_deltau_flag,
            is_mmol=is_mmol_flag,
        )
        print(f"Sending initial series (2009={hex_output})...")
        subprocess.run(
            [
                "pebble",
                "send-app-message",
                "--bytes",
                f"2001={delta_hex}",
                f"2009={hex_output}",
                "--int",
                f"2008={slope_val}",
            ]
        )

        current_step = args.num_points - 1
        prev_raw_value = initial_raw_values[-1]
        save_state(current_step, prev_raw_value, last_timestamp)

    else:
        # 2. Restarted run: compute missed steps and backfill
        now = int(time.time())
        last_ts = state["last_timestamp"]
        elapsed_seconds = max(0, now - last_ts)
        
        # Calculate how many intervals elapsed since last run
        missed_intervals = elapsed_seconds // interval_seconds

        if missed_intervals > 0:
            # Cap backfill points to num_points or the number of missed steps
            backfill_points = min(args.num_points, int(missed_intervals))
            start_step = (state["current_step"] + 1) - (backfill_points - 1)

            hex_output, backfill_values, last_timestamp = generate_series_payload(
                start_step=start_step,
                num_points=backfill_points,
                mmol=args.mmol,
            )

            current_step = state["current_step"] + int(missed_intervals)
            prev_raw_value = backfill_values[-1]
            delta = backfill_values[-1] - (
                backfill_values[-2] if len(backfill_values) > 1 else state["prev_raw_value"]
            )

            if args.send_slope:
                slope_val = slope_ordinal(delta)
            else:
                slope_val = 0

            delta_raw_int, delta_hex = pack_bgl_delta(
                delta_value=delta,
                undefined=0,
                display_units=is_deltau_flag,
                is_mmol=is_mmol_flag,
            )

            print(
                f"Backfilling {backfill_points} missed readings (2009={hex_output})..."
            )
            subprocess.run(
                [
                    "pebble",
                    "send-app-message",
                    "--bytes",
                    f"2001={delta_hex}",
                    f"2009={hex_output}",
                    "--int",
                    f"2008={slope_val}",
                ]
            )

            save_state(current_step, prev_raw_value, last_timestamp)
        else:
            print("No missed intervals since last execution. Resuming live loop...")
            current_step = state["current_step"]
            prev_raw_value = state["prev_raw_value"]

    # 3. Continuous live loop
    while True:
        print(f"\nWaiting {args.interval} minutes for next reading...")
        time.sleep(interval_seconds)

        current_step += 1
        current_time = int(time.time())

        # Calculate new point and raw numerical delta
        curr_raw_value = calculate_sine_val(current_step)
        print(f"curr_raw_value = {curr_raw_value}")
        delta = curr_raw_value - prev_raw_value

        if args.send_slope:
            slope_val = slope_ordinal(delta)
        else:
            slope_val = 0

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
            f"Sending 2002 (--bytes) = {packed_2002} | 2001 (--bytes) = {delta} as {delta_hex} | 2008 (--int) = {slope_val}"
        )

        subprocess.run(
            [
                "pebble",
                "send-app-message",
                "--bytes",
                f"2002={packed_2002}",
                f"2001={delta_hex}",
                "--int",
                f"2008={slope_val}",
            ]
        )

        prev_raw_value = curr_raw_value
        save_state(current_step, prev_raw_value, current_time)


if __name__ == "__main__":
    main()