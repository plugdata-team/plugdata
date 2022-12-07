h(p) = (((p + 1e-20) * 1.2) ** .41) * .91
f(p) = h(p) / (1 - h(p))
g(x, p) = (exp(f(p) * x) - 1) / (exp(f(p)) - 1)
splot [0:1] [0:1] g(x, y)
pause -1
plot [0:1] g(.25, x), g(.5, x), g(.75, x)
pause -1
plot [0:1] g(x, 0), g(x, .25), g(x, .5), g(x, .75), g(x, 1)
print g(.5, 0)
