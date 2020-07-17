import sys

def read_csv(filename):
	text_file = open(filename, "r")
	lines = text_file.readlines()
	x = [l.split() for l in lines]
	x = sorted(x, key=lambda u:u[0])
	return x;
a=read_csv(sys.argv[1])
b=read_csv(sys.argv[2])
if len(a)!=len(b):
	print ("#line does not match")
	sys.exit(1)

for x,y in zip(a,b):
	if len(x)!=len(y):
		print ("#item in line  do not match")
		print(x)
		print(y)
		sys.exit(1)
	x=" ".join(sorted(x[1:]))
	y=" ".join(sorted(y[1:]))
	if x!=y:
		print ("lines do not match")
		print(x)
		print(y)
		sys.exit(1)
