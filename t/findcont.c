/* This file serves as input to findcont and should produce
IF:11 NOCONT:6 SC:3 TC:2
*/
unsigned int a, b;
unsigned int nocont, spc_cont, tab_cont;
int main(int argc, const char **argv) {
	if(a)
		if(b)
			if(a ||
			  b)
				++spc_cont;
	if(argc == 2)
		++nocont;
	if(argc == 2) {
		++nocont;
	}
	if(argc == 2 ||
	  argc == 3)
		++spc_cont;
	if(argc == 2 ||
	  argc == 3) {
		++spc_cont;
	}
	if(argc == 2 ||
		argc == 3)
		++nocont;
	if(argc == 2 ||
		argc == 3) {
		/* previous line (argc==3) regarded as code
		because of bad indent */
		++nocont;
	}
	if(argc == 2 ||
			argc == 3)
		++tab_cont;
	if(argc == 2 ||
			argc == 3) {
		++tab_cont;
	}
	return 0;
}
