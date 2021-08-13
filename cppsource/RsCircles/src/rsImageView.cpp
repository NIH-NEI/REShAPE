
#include "rsimageview.h"

rsImageView::~rsImageView()
{
	if (legend) delete legend;
}

void rsImageView::SetLegendWidth(int w)
{
	legend_width = w;
	if (legend) delete legend;
	legend = new QImage(legend_width, legend_width/6, QImage::Format_RGB888);
	GenerateLegend();
}

inline byte cc(byte *p, byte c, double o) {
	int res = int((*p) * (1. - o) + c * o);
	if (res < 0) res = 0; else if (res > 255) res = 255;
	return (byte)res;
}

void rsImageView::fillParticle(rsParticle &pt, byte r, byte g, byte b)
{
	double o = opacity;
	int x0 = pt.p.xStart - (vsz - 1) / 2;
	int y0 = pt.p.yStart - (vsz - 1) / 2;
	if (o < 1.) {
		for (int y = y0; y < y0 + vsz; y++) {
			byte *p = GetPixelDataPointer(x0, y);
			for (int i = 0; i < vsz; i++) {
				*p++ = cc(p, r, o); *p++ = cc(p, g, o); *p++ = cc(p, b, o);
			}
		}
	}
	else {
		for (int y = y0; y < y0 + vsz; y++) {
			byte *p = GetPixelDataPointer(x0, y);
			for (int i = 0; i < vsz; i++) {
				*p++ = r; *p++ = g; *p++ = b;
			}
		}
	}
}

void rsImageView::reFillParticles()
{
	if (!p_particles) return;
	std::vector<rsParticle> & particles = *p_particles;
	if (par_idx == 1) {
		maxn = 0;
		for (size_t i = 0; i < particles.size(); i++) {
			rsParticle &pt = particles[i];
			int nn = pt.p.neighbors;
			if (maxn < nn) maxn = nn;
			rsRGB & c = glasbey->GetColorAt(nn);
			fillParticle(pt, c.r, c.g, c.b);
		}
	}
	else if (par_idx > 1) {
		size_t i;
		vmin = NaN;
		vmax = NaN;
		for (i = 0; i < particles.size(); i++) {
			rsParticle &pt = particles[i];
			double v = *pt.parPtr(par_off);
			if (std::isnan(v)) continue;
			if (std::isnan(vmin)) {
				vmin = vmax = v;
				continue;
			}
			if (vmin > v) vmin = v;
			if (vmax < v) vmax = v;
		}
		if (vmin == vmax) vmax = vmax + 1.;
		for (i = 0; i < particles.size(); i++) {
			rsParticle &pt = particles[i];
			double v = *pt.parPtr(par_off);
			if (std::isnan(v)) v = vmin;
			int j = (int)(5.5 + (250. * (v - vmin) / (vmax - vmin)));
			rsRGB & c = palette->GetColorAt(j);
			fillParticle(pt, c.r, c.g, c.b);
		}
	}
	else {
		for (size_t i = 0; i < particles.size(); i++) {
			fillParticle(particles[i], 0x00, 0xAA, 0xAA);
		}
	}
	ImageDataModified();
}

void rsImageView::GenerateLegend()
{
	if (par_idx == 0) {
		FillParamLegend(legend, pr, pg, pb);
	}
	else if (par_idx == 1) {
		DrawNeighborsLegend(maxn, legend, glasbey);
	}
	else {
		DrawParamLegend(vmin, vmax, legend, palette);
	}
}

void rsImageView::SetParticles(std::vector<rsParticle> *p_particles)
{
	this->p_particles = p_particles;
	reFillParticles();
	GenerateLegend();
}

void rsImageView::SetVisualParameter(int par_idx)
{
	if (par_idx < 0 || par_idx >= (int)(numVisualParams + 2))
		par_idx = 0;
	this->par_idx = par_idx;
	par_off = 0;
	if (par_idx == 0)
		par_name = "Outlines";
	else if (par_idx == 1)
		par_name = "Neighbors";
	else {
		par_name = visualParams[par_idx - 2].text;
		par_off = visualParams[par_idx - 2].off;
	}
	reFillParticles();
	GenerateLegend();
}

void rsImageView::SetPalette(rsPalette *palette)
{
	this->palette = palette;
	if (par_idx >= 2) {
		reFillParticles();
		GenerateLegend();
	}
}
