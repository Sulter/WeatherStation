#to be run as a cron job as often as new data is added


#open file, we are only interested in the last line
import time

f = open('data.txt', 'r')
newline = ""
for line in f:
    newline = line
f.close()

newline = newline.split(', ')
temp = float(newline[0])
t = int(newline[1])

t = time.strftime("%D %H:%M", time.localtime(t))

temp = round(temp*2)/2

#create the new graph
import subprocess
subprocess.call("gnuplot /home/pi/WeatherStation/plot", shell=True) #this will make it wait until gnuplot finishes

#add text with the current temperature to the graph
from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw

font = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSans.ttf",50)
img=Image.open("/home/pi/WeatherStation/graph.png").convert("RGBA")
draw=ImageDraw.Draw(img)
draw.text((150, 150),"Temp: " + str(temp) + "C",(84,84,84),font=font)
draw.text((150, 250), t ,(84,84,84),font=font)
draw = ImageDraw.Draw(img)
draw = ImageDraw.Draw(img)
img.save("/home/pi/WeatherStation/www/graph.png")


