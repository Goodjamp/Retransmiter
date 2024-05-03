import sys
import re

crcTable = [
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
]

def crc8(data, size, pos):
    crc = 0
    for k in range(0, size):
        crc = crcTable[crc ^ data[pos + k]]
        crc &= 0xFF
    return crc


print("CRSF_TRANSMITER_ADDRESS")

crsfLogTxt = ""
crsfLogHex = []
crsfLogHex = []

print("CRSF_TRANSMITER_ADDRESS")

try:
    file=open("CRSF_log.txt", "r")
    crsfLogTxt = file.read()
    crsfLogTxtHex = re.findall("[0-9,A-F]{2,2}", crsfLogTxt)
    file.close()
except:
    print("Can't open file")

for k in range(0, len(crsfLogTxtHex) - 1):
    crsfLogHex.append(int(crsfLogTxtHex[k], 16))

CRSF_TRANSMITER_ADDRESS = 0xEE
RADIO_TRANSMITER_ADDRESS = 0xEA
FLY_CONTROLLER_ADDRESS = 0xC8
CRSF_RECEIVER_ADDRESS = 0xEC
POS_ADDRESS = 0
POS_LEN = 1
POS_FRAME_TYPE = 2
POS_BEGIN_CRC = 2
MAX_LEN = 64 - 2
MIN_LEN = 4
address = 0
length = 0
type = 0
posCrc = 0
errorCnt = 0
EndPrev = 0
commandArray= []
foundFirst = False

targetCrc = 0
calcCrc = 0


k = 0
targetFrame = 0
if int(sys.argv[1]) == 0:
    targetFrame = CRSF_TRANSMITER_ADDRESS
    print("CRSF_TRANSMITER_ADDRESS")
elif int(sys.argv[1]) == 1:
    targetFrame = RADIO_TRANSMITER_ADDRESS
elif int(sys.argv[1]) == 2:
    targetFrame = FLY_CONTROLLER_ADDRESS
elif int(sys.argv[1]) == 3:
    targetFrame = CRSF_RECEIVER_ADDRESS

while k < len(crsfLogHex):
    if crsfLogHex[k + POS_ADDRESS] == targetFrame:
        address = crsfLogHex[k + POS_ADDRESS]
        if (crsfLogHex[k + POS_LEN] <= MAX_LEN
            and crsfLogHex[k + POS_LEN] >= MIN_LEN
            and crsfLogHex[k + crsfLogHex[k + POS_LEN]] < len(crsfLogHex)):
            length = crsfLogHex[k + POS_LEN]
            posCrc = k + POS_LEN + length
            type =  crsfLogHex[k + POS_FRAME_TYPE]
            targetCrc = crsfLogHex[posCrc]
            calcCrc = crc8(crsfLogHex, length - 1, k + POS_BEGIN_CRC)
            if (targetCrc == calcCrc):
                commandArray.clear()
                for i in range(posCrc + 1, posCrc + 1 + 26):
                    commandArray.append(crsfLogHex[i])
                #print(bytes(commandArray).hex())#print(commandArray)
                #if EndPrev != k:
                #    print("Error: k = {:} EndPrev = {:}".format(k, EndPrev))
                k = posCrc + 1
                EndPrev = k
                foundFirst = True
                print("Find: address = {:02x}, len = {:02x}, type = {:02x}".format(address, length, type))
            else:
                if foundFirst == True:
                    print("Error CRC, address = {:02x}, len = {:02x}, type = {:02x} calcCrc = {:02x} targetCrc = {:02x}".format(address, length, type, calcCrc, targetCrc))
                k = k + 1
        else:
            if foundFirst == True:
                print("Error len")
            k = k + 1
    else:
        #if foundFirst == True:
        #    print("Error address")
        k = k + 1
"""
for k in range(0, len(crsfLogHex) - 4):
    if founFrame == True:
        print("k = ",k)
        founFrame = False
    if crsfLogHex[k + POS_ADDRESS] == CRSF_TRANSMITER_ADDRESS     \
        or crsfLogHex[k + POS_ADDRESS] == RADIO_TRANSMITER_ADDRESS \
        or crsfLogHex[k + POS_ADDRESS] == FLY_CONTROLLER_ADDRESS   \
        or crsfLogHex[k + POS_ADDRESS] == CRSF_RECEIVER_ADDRESS:
        address = crsfLogHex[k + POS_ADDRESS]
        if (crsfLogHex[k + POS_LEN] <= MAX_LEN
            and crsfLogHex[k + POS_LEN] >= MIN_LEN
            and crsfLogHex[k + crsfLogHex[k + POS_LEN]] < len(crsfLogHex)):
            length = crsfLogHex[k + POS_LEN]
            posCrc = k + POS_LEN + length
            if (crc8(crsfLogHex, length - 1, crsfLogHex[k + POS_BEGIN_CRC])\
                == crsfLogHex[posCrc]):
                print("Add = 0x{:02x}".format(crsfLogHex[k]))
                print("Found k = ", k)
                print("length = ", length)
                k = k +  length + 2
                print("After k = ", k)
                founFrame = True
            else:
                errorCnt += 1
                #print("error CRC")
        else:
            errorCnt += 1
            #print("Error len")
    else:
        errorCnt += 1
        #print("Error Address")
"""


#print(len(crsfLogHex))
