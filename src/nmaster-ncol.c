enum {MaxMon = 8};
static int nmasters[MaxMon];
static int initnm = 0;

static void
initnmaster(void) {
	int i;

	if(initnm)
		return;
	for(i = 0; i < MaxMon; i++)
		nmasters[i] = nmaster;
	initnm = 1;
}

static void
incnmaster(const Arg *arg) {
	if(!arg)
		return;
	nmasters[0] += arg->i;
	if(nmasters[0] < 0)
		nmasters[0] = 0;
	arrange();
}

static void
setnmaster(const Arg *arg) {
	if(!arg)
		return;
	nmasters[0] = arg->i > 0 ? arg->i : 0;
	arrange();
}

static void
ntile() {
	int x, y, h, w, mw, nm;
	unsigned int i, n;
	Client *c;

	initnmaster();
	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next), n++);
	c = nexttiled(clients);
	nm = nmasters[0];
	if(nm > n)
		nm = n;
	/* master */
	if(nm > 0) {
		mw = mfact * ww;
		h = wh / nm;
		if(h < bh)
			h = wh;
		y = wy;
		for(i = 0; i < nm; i++, c = nexttiled(c->next)) {
			resize(c, wx, y, (n == nm ? ww : mw) - 2 * c->bw,
			       ((i + 1 == nm) ? wy + wh - y : h) - 2 * c->bw);
			if(h != wh)
				y = c->y + HEIGHT(c);
		}
		n -= nm;
	} else
		mw = 0;
	if(n == 0)
		return;
	/* tile stack */
	x = wx + mw;
	y = wy;
	w = ww - mw;
	h = wh / n;
	if(h < bh)
		h = wh;
	for(i = 0; c; c = nexttiled(c->next), i++) {
		resize(c, x, y, w - 2 * c->bw,
		       ((i + 1 == n) ? wy + wh - y : h) - 2 * c->bw);
		if(h != wh)
			y = c->y + HEIGHT(c);
	}
}

static void
ncol() {
	int x, y, h, w, mw, nm;
	unsigned int i, n;
	Client *c;

	initnmaster();
	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next), n++);
	c = nexttiled(clients);
	nm = nmasters[0];
	if(nm > n)
		nm = n;
	/* master */
	if(nm > 0) {
		mw = (n == nm) ? ww : mfact * ww;
		w = mw / nm;
        x = wx;
		for(i = 0; i < nm; i++, c = nexttiled(c->next)) {
			resize(c, x, wy, w - 2 * c->bw, wh - 2 * c->bw);
            x = c->x + WIDTH(c);
		}
		n -= nm;
	} else
		mw = 0;
	if(n == 0)
		return;
	/* tile stack */
	x = wx + mw;
	y = wy;
	w = ww - mw;
	h = wh / n;
	if(h < bh)
		h = wh;
	for(i = 0; c; c = nexttiled(c->next), i++) {
		resize(c, x, y, w - 2 * c->bw,
		       ((i + 1 == n) ? wy + wh - y : h) - 2 * c->bw);
		if(h != wh)
			y = c->y + HEIGHT(c);
	}
}

static void
nbstack() {
	int x, y, h, w, mh, nm;
	unsigned int i, n;
	Client *c;

	initnmaster();
	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next), n++);
	c = nexttiled(clients);
	nm = nmasters[0];
	if(nm > n)
		nm = n;
	/* master */
	if(nm > 0) {
		mh = mfact * wh;
		w = ww / nm;
		if(w < bh)
			w = ww;
		x = wx;
		for(i = 0; i < nm; i++, c = nexttiled(c->next)) {
			resize(c, x, wy, ((i + 1 == nm) ? wx + ww - x : w) - 2 * c->bw,
			       (n == nm ? wh : mh) - 2 * c->bw);
			if(w != ww)
				x = c->x + WIDTH(c);
		}
		n -= nm;
	} else
		mh = 0;
	if(n == 0)
		return;
	/* tile stack */
	x = wx;
	y = wy + mh;
	w = ww / n;
	h = wh - mh;
	if(w < bh)
		w = ww;
	for(i = 0; c; c = nexttiled(c->next), i++) {
		resize(c, x, y, ((i + 1 == n) ? wx + ww - x : w) - 2 * c->bw,
		       h - 2 * c->bw);
		if(w != ww)
			x = c->x + WIDTH(c);
	}
}
