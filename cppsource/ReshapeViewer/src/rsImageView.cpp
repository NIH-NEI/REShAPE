
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

void rsImageView::fillParticle(rsParticle &pt, byte r, byte g, byte b)
{
	for (size_t i = 0; i < pt.GetFillSize(); i++) {
		rsHSeg s = pt.GetFillAt(i);
		byte *p = GetPixelDataPointer((int)s.x0, (int)s.y);
		if (!p) break;
		for (uint64 x = s.x0; x <= s.x1; x++) {
			*p++ = r;
			*p++ = g;
			*p++ = b;
		}
	}
}

void rsImageView::reFillParticles()
{
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
			fillParticle(particles[i], pr, pg, pb);
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

void rsImageView::SetParticles(std::vector<rsParticle> &srcp, rsRect br)
{
	particles.resize(0);
	for (size_t i = 0; i < srcp.size(); i++) {
		rsParticle & spt = srcp[i];
		if (!br.contains(spt.GetLR()) || !br.contains(spt.GetUL())) continue;
		particles.push_back(rsParticle(spt, br));
	}
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
