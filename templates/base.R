# Automatically generated file
# {{ APP_NAME }} {{ APP_VERSION }}

# install.packages("ggplot2", repos='http://cran.us.r-project.org')
# install.packages("Hmisc", repos='http://cran.us.r-project.org')
# install.packages("tidyverse", repos='http://cran.us.r-project.org')
options(warn = -1)
library(tidyverse)
library(ggplot2)
theme_set(theme_classic())
pdf(NULL)

df <- read.csv({{ csv_file }}, header=TRUE)
names(df) <- {{ csv_headers }}
{% if not primary %}
df$Primary <- paste("")
{% set primary = 'Primary' %}
{% set pri_values = 'c("")' %}
{% endif %}
{% if need_filtering %}
{% if pri_values %}pri_values <- {{ pri_values }}{% endif %}
{% if sec_values %}sec_values <- {{ sec_values }}{% endif %}
{% if pri_values %}
df <- df %>% filter({{primary}} %in% pri_values{% if sec_values %}&{{secondary}} %in% sec_values{% endif %})
{% endif %}
{% endif %}
features <- {{ features }}
ylimits <- matrix({{ ylimits }}, ncol=2)
ylabels <- {{ ylabels }}
filenames <- {{ png_files }}
for (i in 1:length(features)) {
	feature <- features[i]
	y_label <- ylabels[i]

	g <- ggplot(df, aes(x={{ secondary or primary }}, y=df[[feature]], fill={{ primary }})) +
		scale_y_continuous({% if log2 %}trans="log2", {% endif %}limits = ylimits[i,])
	g + geom_{{plot_style}}(aes(fill={{primary}}{% if not blackoutline %}, color={{primary}}{% endif %}),
			alpha={{ boxalpha }}, width=0.81,
			position=position_dodge(1){% if plot_style == 'boxplot' %}, outlier.shape=NA{% endif %}) +
{% if secondary %}		
		geom_point(aes(color={{primary}}), shape=21,
			alpha={{ dotalpha }}, size={{ dotsize }},
			position=position_jitterdodge(jitter.width=0.40, dodge.width=1)) +
{% else %}
		geom_jitter(aes(color={{primary}}), alpha={{ dotalpha }}, size={{ dotsize }}) +
{% endif %}
		labs(x={{ x_label }}, y=y_label, fill={{ primary_quot }}{% if not blackoutline %}, color={{ primary_quot }}{% endif %}) +
{% if annotations %}
		stat_summary(fun.data="mean_sdl", 
			fun.args=list(mult=1.96), 
			geom="errorbar", 
			color="black", 
			position=position_dodge(1), 
			width=0.5) +
		stat_summary(aes(label=paste0("Sd= ", round(..y.., digits=2))),
			fun.y = sd, geom="text", position = position_dodge(1),
			vjust=2.5, size=2.5, colour = "black")+
		stat_summary(aes(fill={{primary}}),
			fun.y=mean, geom="point", color="darkred",
			shape=18, size=3, position = position_dodge(1))+
		stat_summary(aes(label=paste0("Mean= ", round(..y.., digits=2))),
			fun.y=mean, geom="text", position = position_dodge(1),
			vjust=-1.5, size=2.5, colour = "black")+
{% endif %}
		theme(axis.title = element_text(size=rel({{titlesize}})),
			# axis.text.x = element_text(size=rel({{textsize}})),
			# axis.text.y = element_text(size=rel({{textsize}})),
			# axis.ticks.y = element_line(colour = "black"),
			# axis.line.y = element_line(colour = "black"),
			panel.grid.major.y = element_line(colour = "grey96"),
{% if secondary %}
			legend.title = element_text(size=rel({{textsize}})),
			legend.text = element_text(size=rel({{textsize}})),
			legend.position = "right",
			legend.justification = "center",
			legend.box.background = element_rect(colour = "black", fill = "grey96"),
{% else %}
			legend.position = "none",
{% endif %}
			strip.text.y = element_text(size=rel({{titlesize}}), margin = margin(0,0,0,0, "cm")),
			strip.background = element_rect(colour = "grey96", fill="grey96"),
			# panel.background = element_rect(fill = "grey96"),
			plot.background = element_rect(fill = "grey96")
		) +
		guides(colour = {% if blackoutline %}FALSE{% else %}g{% endif %})
	
	fname <- filenames[i]
	print(paste(feature, fname, sep="|"), quote = FALSE)
	ggsave(fname, 
	       width = {{ width }}, 
	       height = {{ height }}, 
	       dpi = 300, 
	       units="in")
}
