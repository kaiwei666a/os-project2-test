
static unsigned int seed = 1;
void srand(unsigned int s) {
  seed = s;
}

int
get_random(int min, int max)
{

  seed = (1664525 * seed + 1013904223);
  unsigned int rand = (seed >> 16) & 0x7FFF; 
  int range = max - min;
  return (rand % range) + min;  
}
