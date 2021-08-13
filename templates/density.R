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
{% if pri_values %}
pri_values <- {{ pri_values }}
{% set pcolor = secondary or '1' %}
{% if sec_values %}
sec_values <- {{ sec_values }}
p1 <- df %>% filter({{primary}} %in% pri_values)
s1 <- df %>% filter({{secondary}} %in% sec_values)
s_len <- nrow(s1)

s1 <- s1[rep(seq_len(s_len), length(pri_values)), ]
s1${{primary}} <- NULL
{{primary}} <- rep(pri_values, each=s_len)

s1 <- data.frame(s1, {{primary}})
df <- bind_rows(s1,p1)
{% else %}
df <- df %>% filter({{primary}} %in% pri_values)
{% endif %}
{% endif %}

features <- {{ features }}
ylimits <- matrix({{ ylimits }}, ncol=2)
ylabels <- {{ ylabels }}
filenames <- {{ png_files }}
for (i in 1:length(features)) {
	feature <- features[i]
	y_label <- ylabels[i]

	ggplot(df, aes(x=df[[feature]], fill={{pcolor}}{% if not blackoutline %}, color={{pcolor}}{% endif %})) +
		scale_x_continuous({% if log2 %}trans="log2", {% endif %}limits = ylimits[i,]) +
	geom_density(aes(y=..scaled..), alpha = {{ boxalpha }}) +
	facet_grid({{primary}}~., switch = "y") +
	labs(x=y_label, y={{primary_quot}}, fill="Legend"{% if not blackoutline %}, color="Legend"{% endif %}) +
	theme(axis.title= element_text(size=rel({{titlesize}}), hjust=1, vjust=1), # size=rel(2.)
		axis.text.x= element_text(size=rel({{textsize}})), # size=rel(1.3)
		axis.text.y = element_blank(),
		axis.ticks.y = element_blank(),
		axis.line.y = element_blank(),
{% if secondary %}
		legend.title = element_text(size=rel({{textsize}})), # size=rel(1.3)
		legend.text = element_text(size=rel({{legendsize}})), # size=rel(1.3)
		legend.position = "bottom",
		legend.justification = "left",
		legend.box.background = element_rect(colour = "black", fill = "grey96"),
{% else %}
		legend.position = "none",
{% endif %}
		strip.text.y = element_text(size=rel({{titlesize}}), angle=180, margin = margin(0,0,0,0, "cm")), # size=20
		strip.background = element_rect(colour = "grey96", fill="grey96"),
		panel.background = element_rect(fill = "grey96"),
		plot.background = element_rect(fill = "grey96")
	)
	
	fname <- filenames[i]
	print(paste(feature, fname, sep="|"), quote = FALSE)
	ggsave(fname, 
	       width = {{ width }}, 
	       height = {{ height }}, 
	       dpi = 300, 
	       units="in")
}
