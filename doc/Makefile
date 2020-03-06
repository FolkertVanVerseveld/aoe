.PHONY: default run clean

GFX=\
	images/aoe_cover.jpg \
	images/gamesao_2vs2.jpg \
	images/pe_layout.png \
	images/pe_restree.png \
	images/pe_format.png \
	images/openage.png

OBJECTS=\
	pe_format.pdf \
	$(GFX)

default: $(OBJECTS)

images/aoe_cover.jpg:
	wget -O images/aoe_cover.jpg https://giantbomb1.cbsistatic.com/uploads/original/1/17557/1131792-age_of_empires_user.jpg
images/gamesao_2vs2.jpg:
	wget -O images/gamesao_2vs2.jpg http://photoservice.gamesao.vn/Resources/Upload/Images/Editor/30/%C4%90%E1%BA%A1i%20chi%E1%BA%BFn%20AOE%20Vi%E1%BB%87t%20-%20Trung%20-%20Chim%20S%E1%BA%BB%20%C4%90i%20N%E1%BA%AFng%20t%C3%A1i%20%C4%91%E1%BA%A5u%20Shen%20Long%20th%E1%BB%83%20lo%E1%BA%A1i%202vs2/chim-se-di-nang-vs-shen-long-3480-1433995161.jpg
images/pe_layout.gif:
	wget -O images/pe_layout.gif http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/pef2034b.gif
images/pe_layout.png: images/pe_layout.gif
	convert images/pe_layout.gif images/pe_layout.png
images/pe_restree.gif:
	wget -O images/pe_restree.gif http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/pef2034c.gif
images/pe_restree.png: images/pe_restree.gif
	convert images/pe_restree.gif images/pe_restree.png
images/openage.png:
	wget -O images/openage.png https://camo.githubusercontent.com/a618e8cc7c30c8d926ff75938037bce077b24243/68747470733a2f2f6f70656e6167652e7366742e6d782f706963732f6f70656e6167655f322e706e67

via.pdf: via.tex $(OBJECTS)
	latexmk -pdf via.tex
run: via.pdf
	xdg-open via.pdf
pe_format.pdf:
	wget -O pe_format.pdf http://www.openrce.org/reference_library/files/reference/PE%20Format.pdf
images/pe_format.png: pe_format.pdf
	convert pe_format.pdf images/pe_format.png

clean:
	rm -f *.aux *.log *.toc via.pdf $(OBJECTS)
