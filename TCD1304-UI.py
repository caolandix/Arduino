import serial
import time
from Tkinter import *
from ttk import *


class Application(Frame):
    def __init__(self, master=None, port=None, exposure=50):
        Frame.__init__(self, master)
        self.parent = master
        self.pack()
        self.result = {}
        self.entry_text = ''
        self.dark = [0] * 3648
        self.exposure = StringVar()
        self.exposure.set(str(exposure))
        self.port = port
        self.createWidgets()
        self.ser = serial.Serial(self.port, 115200, timeout=3)

    def createWidgets(self):

        self.TOP = Frame(self)
        self.TOP.pack()

        self.RUN = Button(self.TOP)
        self.RUN["text"] = "Sample"
        self.RUN["command"] = self.run
        self.RUN.pack({"side": "left"})

        self.CLEAR = Button(self.TOP)
        self.CLEAR["text"] = "Clear"
        self.CLEAR["command"] = self.clear
        self.CLEAR.pack({"side": "left"})

        self.QUIT = Button(self.TOP)
        self.QUIT["text"] = "Quit"
        self.QUIT["command"] = self.quit
        self.QUIT.pack({"side": "left"})

        self.LABEL_SAMPLES = Label(self.TOP, text="Samples:")
        self.LABEL_SAMPLES.pack({"side": "left"})
        self.sample_count = IntVar()
        self.SAMPLES_1 = Radiobutton(self.TOP, value=1, variable=self.sample_count, text="1", width=4)
        self.SAMPLES_2 = Radiobutton(self.TOP, value=2, variable=self.sample_count, text="2", width=4)
        self.SAMPLES_3 = Radiobutton(self.TOP, value=3, variable=self.sample_count, text="3", width=4)
        self.SAMPLES_4 = Radiobutton(self.TOP, value=4, variable=self.sample_count, text="4", width=4)
        self.SAMPLES_1.pack({"side": "left"})
        self.SAMPLES_2.pack({"side": "left"})
        self.SAMPLES_3.pack({"side": "left"})
        self.SAMPLES_4.pack({"side": "left"})
        self.SAMPLES_1.invoke()

        self.EXPOSURE = Entry(self.TOP, textvariable=self.exposure)
        self.EXPOSURE.pack({"side": "left"})

        self.SETEXP = Button(self.TOP)
        self.SETEXP["text"] = "Exposure"
        self.SETEXP["command"] = self.send_exposure
        self.SETEXP.pack({"side": "left"})

        self.BASELINE = Button(self.TOP)
        self.BASELINE["text"] = "Baseline"
        self.BASELINE["command"] = self.baseline
        self.BASELINE.pack({"side": "left"})

        self.canvas = Canvas(self, bg="white", height=258, width=1216)
        self.canvas.pack({"side": "right"})

        self.blank_graph()

    def baseline(self):

        self.dark = [0] * 3648

        self.ser.write("e0\n")
        time.sleep(0.1)

        for x in range(0, 1):
            self.ser.write("r\n")

            chars = ''
            while True:
                char = self.ser.read()
                if char == '':
                    break
                chars += char

            data = chars.split("\n")

            for x in data:
                x = x.strip()
                if x == '':
                    continue
                y = x.split(",")

                if len(y) < 2:
                    continue
                if y[0] == '' or y[1] == '':
                    continue
                a = int(y[0])
                b = int(y[1])

                if b > 0:
                    self.dark[a] += b
            for a in range(0, 3648):
                self.dark[a] /= 2

        self.send_exposure()

    def send_exposure(self):

        exp = self.exposure.get()
        try:
            exposure = int(exp)
        except Exception as e:
            print e.message
            exposure = 20

        if exposure > 1000:
            exposure = 1000
            self.exposure.set("1000")

        exp = "e" + str(exposure) + "\n"
        self.ser.write(exp)

    def run(self):

        buckets = [0] * 3649
        counts = [0] * 3649
        self.result = {}

        if self.sample_count.get() == 0:
            self.sample_count.set(1)

        for t in range(0, self.sample_count.get()):
            time.sleep(0.1)
            self.ser.write("r\n")
            chars = ''
            while True:
                char = self.ser.read()
                if char == '':
                    break
                chars += char

            data = chars.split("\n")

            for x in data:
                x = x.strip()
                if x == '':
                    continue
                y = x.split(",")

                if len(y) < 2:
                    continue
                if y[0] == '' or y[1] == '':
                    continue
                a = int(y[0])
                b = int(y[1])

                if b > 0:
                    counts[a] += 1
                    buckets[a] += b

        output = []
        for x in range(0, 3648):
            if counts[x] > 0:
                val = buckets[x] / counts[x]
            else:
                val = buckets[x]
            output.append(val)
        fp = open("ccd.csv", "w")
        for x in range(1, 3648):
            y = int(x / 3)
            if y in self.result:
                self.result[y] += (output[x] - self.dark[x]) / 3
            else:
                self.result[y] = (output[x] - self.dark[x]) / 3

            fp.write(str(x) + "," + str(output[x]) + "\n")
        fp.close()
        self.draw_graph()

    def clear(self):
        self.canvas.delete("line")
        self.blank_graph()

    def blank_graph(self):

        for x in range(0, 1215, 25):
            self.canvas.create_line(x, 0, x, 255, fill="#eeeeee", width=1)
        for y in range(0, 255, 25):
            self.canvas.create_line(0, y, 1215, y, fill="#eeeeee", width=1)

        self.canvas.create_line(3, 3, 3, 255, fill="#555555", width=1)
        self.canvas.create_line(3, 255, 1215, 255, fill="#555555", width=1)
        self.canvas.create_line(1215, 255, 1215, 0, fill="#555555", width=1)
        self.canvas.create_line(1215, 3, 3, 3, fill="#555555", width=1)

    def draw_graph(self):

        for x in range(0, 1215):
            x1 = x
            y1 = 255 - int(self.result[x1])
            x2 = x + 1
            y2 = 255 - int(self.result[x2])
            self.canvas.create_line(x1, y1, x2, y2, fill="#000000", width=1)
        self.canvas.addtag_all("line")


root = Tk()
app = Application(master=root, port="/dev/ttyACM1", exposure=50)
app.master.title("TCD1304 Sampler")
app.mainloop()
app.ser.close()
root.destroy()
