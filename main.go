package main

import (
    "bufio"
    "fmt"
    "go.bug.st/serial.v1"
    "log"
    "os"
    "strconv"
    "strings"
)

func main() {
    ports, err := serial.GetPortsList()
    if err != nil {
        log.Fatal(err)
    }

    if len(ports) == 0 {
        log.Fatal("No serial ports found!")
    }

    var selectedPort string = ""

    reader := bufio.NewReader(os.Stdin)

    for selectedPort == "" {
        fmt.Println("Please select a port:")
        for i, port := range ports {
            fmt.Printf("[%2d] %v\n", i+1, port)
        }

        text, _ := reader.ReadString('\n')
        text = strings.Replace(text, "\n", "", -1)
        text = strings.Replace(text, "\r", "", -1)

        if text == "" {
            fmt.Println("You didn't pick anything.  Try again.")
        } else {
            index, err := strconv.Atoi(text)
            if err != nil {
                fmt.Printf("'%s' doesn't appear to be a number.  Try again.\n", text)
            } else if index < 1 || index > len(ports) {
                fmt.Printf("You selected an invalid value, '%d'.  Try agian.", index)
            } else {
                selectedPort = ports[index-1]
            }
        }
    }

    fmt.Println("Opening port: ", selectedPort)

    mode := &serial.Mode{
        BaudRate: 9600,
    }
    port, err := serial.Open(selectedPort, mode)
    if err != nil {
        log.Fatalln(err)
    }

    defer port.Close()

    fmt.Println("\n-----\nPlease set the throttle to 0 and hit enter.")
    _, _ = reader.ReadString('\n')
    output := []byte{0x01, 0x00}
    writeData(port, output)
    fmt.Println("Please set the throttle to 25% and hit enter.")
    _, _ = reader.ReadString('\n')
    output[1] = 0x01
    writeData(port, output)
    fmt.Println("Please set the throttle to 50% and hit enter.")
    _, _ = reader.ReadString('\n')
    output[1] = 0x02
    writeData(port, output)
    fmt.Println("Please set the throttle to 75% and hit enter.")
    _, _ = reader.ReadString('\n')
    output[1] = 0x03
    writeData(port, output)
    fmt.Println("Please set the throttle to 100% and hit enter.")
    _, _ = reader.ReadString('\n')
    output[1] = 0x04
    writeData(port, output)

    fmt.Println("Calibration is complete.")
}

func writeData(port serial.Port, output []byte) {
    responseBuffer := make([]byte, 1)
    _, err := port.Write(output)
    checkAndLog(err)
    _, err = port.Read(responseBuffer)
    checkAndLog(err)
    if responseBuffer[0] != 0x06 {
        log.Fatalf("Recieved non-success reply from throttle.")
    }
}

func checkAndLog(err error) {
    if err != nil {
        log.Fatalln(err)
    }
}
