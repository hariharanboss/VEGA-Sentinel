import argparse
import csv
import sys
import threading
from datetime import datetime

import serial


DEFAULT_LABEL = "normal"
DEFAULT_EXPERIMENT = "baseline"
DEFAULT_PHASE = "baseline"

LABEL_KEYS = {
    "n": "normal",
    "w": "warning",
    "c": "critical",
}

PHASE_KEYS = {
    "b": "baseline",
    "v": "event",
    "r": "recovery",
}


def input_listener(state):
    while not state["stop"]:
        try:
            command = input().strip().lower()

        except EOFError:
            state["stop"] = True
            break

        if command == "q":
            state["stop"] = True
            print("\n[logger] Stopping...")

        elif command in LABEL_KEYS:
            state["label"] = LABEL_KEYS[command]

            print(
                f"\n[logger] Label changed to: "
                f"{state['label']}"
            )

        elif command in PHASE_KEYS:
            state["phase"] = PHASE_KEYS[command]

            print(
                f"\n[logger] Phase changed to: "
                f"{state['phase']}"
            )

        elif command.startswith("e "):
            experiment = command[2:].strip()

            if experiment:
                state["experiment"] = experiment

                print(
                    f"\n[logger] Experiment changed to: "
                    f"{experiment}"
                )

            else:
                print(
                    "\n[logger] Experiment name "
                    "cannot be empty."
                )

        elif command:
            print(
                "\n[logger] Commands:"
                "\n  n + Enter          = normal"
                "\n  w + Enter          = warning"
                "\n  c + Enter          = critical"
                "\n  b + Enter          = baseline phase"
                "\n  v + Enter          = event phase"
                "\n  r + Enter          = recovery phase"
                "\n  e <name> + Enter   = change experiment"
                "\n  q + Enter          = quit"
            )


def validate_fields(fields):

    # Expected ARIES CSV:
    # timestamp_ms,temp_c,humidity_pct,mq2_adc,flame_state,dht_ok

    if len(fields) != 6:
        return False

    try:
        int(fields[0])

        temp = float(fields[1])
        humidity = float(fields[2])

        mq2 = int(fields[3])
        flame = int(fields[4])
        dht_ok = int(fields[5])

    except ValueError:
        return False

    # Validate DHT22
    if dht_ok == 1:

        if not (-40 <= temp <= 80):
            return False

        if not (0 <= humidity <= 100):
            return False

    elif dht_ok == 0:

        if temp != -999 or humidity != -999:
            return False

    else:
        return False

    # Flame sensor must be 0 or 1
    if flame not in (0, 1):
        return False

    # VEGA ARIES v2.0 analogRead()
    # currently returns ADS1015 12-bit
    # single-ended positive range: 0-2047
    if not (0 <= mq2 <= 2047):
        return False

    return True


def main():

    parser = argparse.ArgumentParser(
        description="VEGA Sentinel v0.4 dataset logger"
    )

    parser.add_argument(
        "--port",
        required=True,
        help="Serial port, for example COM19"
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Serial baud rate"
    )

    parser.add_argument(
        "--label",
        default=DEFAULT_LABEL,
        choices=["normal", "warning", "critical"],
        help="Initial label"
    )

    parser.add_argument(
        "--experiment",
        default=DEFAULT_EXPERIMENT,
        help="Initial experiment name"
    )

    parser.add_argument(
        "--phase",
        default=DEFAULT_PHASE,
        choices=["baseline", "event", "recovery"],
        help="Initial experiment phase"
    )

    args = parser.parse_args()

    start_time = datetime.now()

    timestamp_string = start_time.strftime(
        "%Y%m%d_%H%M%S"
    )

    out_filename = (
        f"sentinel_data_{timestamp_string}.csv"
    )

    rejected_filename = (
        f"sentinel_rejected_{timestamp_string}.csv"
    )

    # Open serial port
    try:

        ser = serial.Serial(
            args.port,
            args.baud,
            timeout=2
        )

    except serial.SerialException as e:

        print(
            f"[logger] ERROR: Could not open "
            f"{args.port}"
        )

        print(f"[logger] {e}")

        sys.exit(1)

    print("\n================================================")
    print("          VEGA SENTINEL v0.4 LOGGER")
    print("================================================")

    print(f"Serial port : {args.port}")
    print(f"Baud rate   : {args.baud}")
    print(f"Output file : {out_filename}")
    print(f"Rejected    : {rejected_filename}")
    print(f"Label       : {args.label}")
    print(f"Experiment  : {args.experiment}")
    print(f"Phase       : {args.phase}")

    print("================================================\n")

    print("Controls:")
    print("  n + Enter          -> normal")
    print("  w + Enter          -> warning")
    print("  c + Enter          -> critical")
    print("  b + Enter          -> baseline")
    print("  v + Enter          -> event")
    print("  r + Enter          -> recovery")
    print("  e <name> + Enter   -> change experiment")
    print("  q + Enter          -> quit\n")

    state = {
        "label": args.label,
        "experiment": args.experiment,
        "phase": args.phase,
        "stop": False
    }

    listener = threading.Thread(
        target=input_listener,
        args=(state,),
        daemon=True
    )

    listener.start()

    rows_written = 0
    rows_rejected = 0
    header_seen = False

    # Main valid-data CSV
    with open(
        out_filename,
        "w",
        newline="",
        encoding="utf-8"
    ) as file, open(
        rejected_filename,
        "w",
        newline="",
        encoding="utf-8"
    ) as rejected_file:

        writer = csv.writer(file)

        rejected_writer = csv.writer(
            rejected_file
        )

        # Valid dataset header
        writer.writerow([
            "laptop_timestamp",
            "timestamp_ms",
            "temp_c",
            "humidity_pct",
            "mq2_adc",
            "flame_state",
            "dht_ok",
            "label",
            "experiment",
            "phase"
        ])

        # Rejected data header
        rejected_writer.writerow([
            "laptop_timestamp",
            "raw_line",
            "reason"
        ])

        file.flush()
        rejected_file.flush()

        print(
            "[logger] Waiting for ARIES data...\n"
        )

        while not state["stop"]:

            try:

                raw_line = (
                    ser.readline()
                    .decode(
                        "utf-8",
                        errors="replace"
                    )
                    .strip()
                )

            except serial.SerialException as e:

                print(
                    f"\n[logger] Serial error: {e}"
                )

                break

            if not raw_line:
                continue

            # Detect ARIES CSV header
            if raw_line.startswith("timestamp_ms"):

                if not header_seen:

                    header_seen = True

                    print(
                        "[logger] ARIES CSV header detected."
                    )

                    print(
                        "[logger] Dataset recording "
                        "started.\n"
                    )

                continue

            fields = raw_line.split(",")

            # Validate sensor row
            if not validate_fields(fields):

                rows_rejected += 1

                rejection_time = (
                    datetime.now().isoformat(
                        timespec="milliseconds"
                    )
                )

                rejected_writer.writerow([
                    rejection_time,
                    raw_line,
                    "validation_failed"
                ])

                rejected_file.flush()

                print(
                    f"[logger] REJECTED: "
                    f"{raw_line}"
                )

                continue

            # Timestamp from laptop
            laptop_timestamp = (
                datetime.now().isoformat(
                    timespec="milliseconds"
                )
            )

            # Preserve raw ARIES values
            row = [
                laptop_timestamp,
                fields[0],
                fields[1],
                fields[2],
                fields[3],
                fields[4],
                fields[5],
                state["label"],
                state["experiment"],
                state["phase"]
            ]

            writer.writerow(row)

            file.flush()

            rows_written += 1

            print(
                f"[{rows_written:05d}] "
                f"[{state['label']}] "
                f"[{state['experiment']}] "
                f"[{state['phase']}] "
                f"{raw_line}"
            )

    ser.close()

    print("\n================================================")
    print("              LOGGER FINISHED")
    print("================================================")

    print(f"Rows written   : {rows_written}")
    print(f"Rows rejected  : {rows_rejected}")
    print(f"Output file    : {out_filename}")
    print(f"Rejected file  : {rejected_filename}")

    print("================================================\n")


if __name__ == "__main__":
    main()