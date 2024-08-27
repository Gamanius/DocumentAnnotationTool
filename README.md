# Document Annotation Tool
Docanto (**Doc**ument **An**notation **To**ol) is designed to view and annotate PDF with performance and usability in mind. It uses [mupdf](https://github.com/ArtifexSoftware/mupdf) as it's PDF parser and Direct2D to render the PDFs. To maximize compatibility with the Windows OS, the program uses Win32 calls directly.
Currently, this is a one-man project, so any updates will be sporadic.

Only Windows is supported, with no intention to port it to other operating systems.
## Rough status of the project
- [x] **Viewing PDFs**: PDFs can currently be opened and viewed. Due to the efficient use of multithreading and caching, the program will stay responsive and only re-render pages if needed. Additionally, pages can be moved around the canvas.
- [ ] **Annotating PDFs**: It will be possible to create ink annotations of any size and colour.
- [ ] **Opening and saving multiple PDFs**: It should be possible to have multiple PDFs open inside the program. Additionally, saving and autosaving multiple PDFs will be no problem.
- [ ] **Creation of PDFs**: It should be possible to easily create a blank PDF which can be freely be annotated. Additionally, creating PDFs from a template should also be possible.
- [ ] **Managing PDFs**: It should be easy to rearrange, insert or remove PDF pages.
- [ ] **OCR**: Using an open-source OCR-Engine, OCR capabilities should be directly built into the application.
## TODO List
This project is very much a work in progress, so many features are still being worked on:
1. Annotating PDF
2. Opening multiple PDFs
3. Search in PDFs
4. Saving PDFs
5. Creating new PDFs
6. Better touch support
7. Better Logging and Logging class
### Licence
<p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/"><span property="dct:title">Docanto</span> is licensed under <a href="https://creativecommons.org/licenses/by-nc-sa/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer" style="display:inline-block;">CC BY-NC-SA 4.0<img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/cc.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/by.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/nc.svg?ref=chooser-v1" alt=""><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;" src="https://mirrors.creativecommons.org/presskit/icons/sa.svg?ref=chooser-v1" alt=""></a></p>
