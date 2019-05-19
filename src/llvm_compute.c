// C file to define function to be computed

struct Input { int r1; int g1; int b1; int r2; int g2; int b2; } input;
struct Output{ int o; } output;

void compute()
{
  int avg1, avg2, diff;

  avg1 = input.r1 + input.g1 + input.b1;
  avg2 = input.r2 + input.g2 + input.b2;

  diff = avg2-avg1;

  output.o = diff-4;
}
