#install pyserial and argparse
import argparse
import serial
import os

parser = argparse.ArgumentParser(description='Sniffer to save serial data on text file.')
parser.add_argument('--serial', dest='serial_port', default="/dev/ttyACM0", help='Serial port')
parser.add_argument('--baud', dest='baudrate', default=115200, help='Serial baudrate')
parser.add_argument('--out', dest='out', default="output.txt", help='Complete path to file output')
parser.add_argument('--rows', dest='rows', default=-1, help='Number of lines to save into output file (default infinite)')
args = parser.parse_args()

if __name__ == "__main__":
    print("Starting recording")
    ser = serial.Serial(
        port=args.serial_port,
        baudrate=args.baudrate,
    )

 

    if not ser.is_open:
        print("Error opening the Seril port")
        exit(1)

    if os.path.exists(args.out):
        append_write = 'a' # append if already exists
    else:
        append_write = 'w' # make a new file if not

    try:
        outputFile = open(args.out,append_write)
    except OSError:
        print("Error on opening or creating the output file")
        ser.close()
        exit(1)

    rowSaved = 0

    while True:
        line = (ser.readline()).decode('utf-8')
        outputFile.write(line)
        rowSaved += 1
        print(line)
        if int(args.rows) > 0:
            print(f"Saving: {((rowSaved / int(args.rows))*100)} %")
        else:
             print(f"Saved : {rowSaved}")
        if int(args.rows) > 0 and int(args.rows) <= rowSaved:
            break
    
    ser.close()
    outputFile.close()

    print("Finished")