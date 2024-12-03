import os

a = open("gag.txt", "r")
b = open("gag1.txt", "")

c = a.readlines()
for i in c:
    if i=='\n':
        continue
    b.write(i)