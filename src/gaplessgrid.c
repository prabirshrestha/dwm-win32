void
gaplessgrid() {
	unsigned int n, cols, rows, cn, rn, i, cx, cy, cw, ch;
	Client *c;

	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next))
		n++;
	if(n == 0)
		return;

	/* grid dimensions */
	for(cols = 0; cols <= n/2; cols++)
		if(cols*cols >= n)
			break;
	if(n == 5)		/* set layout against the general calculation: not 1:2:2, but 2:3 */
		cols = 2;
	rows = n/cols;

	/* window geometries (cell height/width/x/y) */
	cw = ww / (cols ? cols : 1);
	cn = 0; 			/* current column number */
	rn = 0; 			/* current row number */
	for(i = 0, c = nexttiled(clients); c; c = nexttiled(c->next)) {
		if(i/rows+1 > cols-n%cols)
			rows = n/cols+1;
		ch = wh / (rows ? rows : 1);
		cx = wx + cn*cw;
		cy = wy + rn*ch;
		resize(c, cx, cy, cw - 2 * c->bw, ch - 2 * c->bw);
		
		i++;
		rn++;
		if(rn >= rows) { 	/* jump to the next column */
			rn = 0;
			cn++;
		}
	}
}
