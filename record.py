import matplotlib
matplotlib.use('TkAgg')
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

from collections import deque
import threading
import sys

MAXLEN = 200
EVENT_HEIGHT = 1500         

x_buf = deque(maxlen=MAXLEN)
y_buf = deque(maxlen=MAXLEN)
z_buf = deque(maxlen=MAXLEN)
event_buf = deque(maxlen=MAXLEN)
time_buf = deque(maxlen=MAXLEN)

data_lock = threading.Lock()


def reader():
    pending_event = False
    try:
        for line in sys.stdin:
            line = line.strip()

            if "EVENT" in line:
                pending_event = True
                print("Significant motion detected")
                continue


            x, y, z, t = (int(v) for v in line.split(","))
            with data_lock:
                x_buf.append(x)
                y_buf.append(y)
                z_buf.append(z)
                time_buf.append(t)
                event_buf.append(EVENT_HEIGHT if pending_event else 0)
            pending_event = False

    except KeyboardInterrupt:
        print("\nEnd of Data Collection\n")

    finally:
        with data_lock:
            df = pd.DataFrame({
                "x": list(x_buf),
                "y": list(y_buf),
                "z": list(z_buf),
                "time": list(time_buf),
                "event": list(event_buf),
            })
        df.to_csv("record_data.csv", index=False)


def main():
    threading.Thread(target=reader, daemon=True).start()

    fig, ax = plt.subplots()
    x_line, = ax.plot([], [], label='X')
    y_line, = ax.plot([], [], label='Y')
    z_line, = ax.plot([], [], label='Z')
    e_line, = ax.plot([], [], label='Sig. motion', color='red', linewidth=2)
    
    ax.legend(loc='upper left')
    ax.set_xlabel("time (ms)")
    ax.set_ylabel("acceleration (mg)")

    def update(frame):
        with data_lock:
            if len(time_buf) > 0:
                t = list(time_buf)
                x_line.set_data(t, list(x_buf))
                y_line.set_data(t, list(y_buf))
                z_line.set_data(t, list(z_buf))
                e_line.set_data(t, list(event_buf))
        ax.relim()
        ax.autoscale_view()
        return x_line, y_line, z_line, e_line

    ani = FuncAnimation(fig, update, interval=50, cache_frame_data=False)
    plt.show()


if __name__ == '__main__':
    main()