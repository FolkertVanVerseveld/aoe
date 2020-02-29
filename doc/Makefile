.PHONY: default run clean

GFX=\
	images/aoe_cover.jpg \
	images/gamesao_2vs2.jpg

default: via.pdf $(GFX)

images/aoe_cover.jpg:
	wget -O images/aoe_cover.jpg https://giantbomb1.cbsistatic.com/uploads/original/1/17557/1131792-age_of_empires_user.jpg
images/gamesao_2vs2.jpg:
	wget -O images/gamesao_2vs2.jpg http://photoservice.gamesao.vn/Resources/Upload/Images/Editor/30/%C4%90%E1%BA%A1i%20chi%E1%BA%BFn%20AOE%20Vi%E1%BB%87t%20-%20Trung%20-%20Chim%20S%E1%BA%BB%20%C4%90i%20N%E1%BA%AFng%20t%C3%A1i%20%C4%91%E1%BA%A5u%20Shen%20Long%20th%E1%BB%83%20lo%E1%BA%A1i%202vs2/chim-se-di-nang-vs-shen-long-3480-1433995161.jpg

via.pdf: via.tex $(GFX)
	pdflatex via.tex
run: via.pdf
	xdg-open via.pdf

clean:
	rm -f *.aux *.log *.toc $(GFX)
