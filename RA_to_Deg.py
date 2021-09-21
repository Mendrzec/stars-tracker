#Wega (Fidis - Harp star)
#RA/Dekl (J2000.0): 18h36m56.51s/+38°47'08.7"

#Altair
#20h12m02.00s/+7°41'04.0"
ra = "18h36m56.51s"
hours, ra = ra.split('h')
minutes, ra = ra.split('m')
seconds, ra = ra.split('s')
degrees = float(hours) * 15
minutes_deg = float(minutes) * 15
seconds_deg = float(seconds) * 15
dec_degress = degrees + minutes_deg / 60 + seconds_deg / 3600
print(dec_degress)

print(int(dec_degress / 15))
dec_degress %= 15
print(int(dec_degress / (15/60)))
dec_degress %= (15/60)
print(dec_degress / (15/3600))


# movetodeg 0 8.87 100
# movetodeg -24.2 38.78 100

movetodeg 228 8.9 1000
movetodeg 204.2 38.8 1000

# RA = x - 75deg
