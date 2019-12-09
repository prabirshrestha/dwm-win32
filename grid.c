void
grid() {
	unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
	Client *c;

	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next))
		n++;

	/* grid dimensions */
	for(rows = 0; rows <= n/2; rows++)
		if(rows*rows >= n)
			break;
	cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

	/* window geoms (cell height/width) */
	ch = wh / (rows ? rows : 1);
	cw = ww / (cols ? cols : 1);
	for(i = 0, c = nexttiled(clients); c; c = nexttiled(c->next)) {
		cx = wx + (i / rows) * cw;
		cy = wy + (i % rows) * ch;
		/* adjust height/width of last row/column's windows */
		ah = ((i + 1) % rows == 0) ? wh - ch * rows : 0;
		aw = (i >= rows * (cols - 1)) ? ww - cw * cols : 0;
		resize(c, cx, cy, cw - 2 * c->bw + aw, ch - 2 * c->bw + ah);
		i++;
	}
}
