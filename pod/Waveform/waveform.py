import pygame
from serial import Serial

pygame.init()
(width, height) = (300, 200)
screen = pygame.display.set_mode((width, height))
screen.fill((255,255,255))
pygame.display.update()

dead=False

daisy = Serial('/dev/tty.usbmodem3464358231391')

def encode32(value):
  return [value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF]

def decode16(values):
  return values[0] | (values[1] << 8)

frequency = 0
refreshRate = 20
bufferSize = width
daisy.write(bytes(encode32(frequency) + encode32(refreshRate) + encode32(bufferSize)))

def transform(y, min, max):
  return (1.0 - ((y - min) / (max - min))) * height

def fetchData():
  return daisy.read(decode16(daisy.read(2)))

def draw(data):
  (_min, _max) = (min(data), max(data))
  screen.fill((255,255,255))
  old = (0, transform(data[0], 0, 255))
  for x, y in enumerate(data):
    new = (x / (len(data) - 1) * width, transform(y, 0, 255))
    pygame.draw.line(screen, (255, 0, 0), old, new, 1)
    old = new
  pygame.display.update()

while(dead==False):
  # pygame.time.wait(int(1000.0/refreshRate))
  draw(fetchData())
  for event in pygame.event.get(): 
    if event.type == pygame.QUIT:
      dead = True
