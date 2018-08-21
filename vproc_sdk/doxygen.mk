#
# Microsemi VPROC API .
#
# Copyright (c) 2002-2015 Microsemi Corporations "Microsemi". All
# Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted. 
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
# INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR USE AND NON-INFRINGEMENT.
#
include Makefile.globals
include config.mk

.PHONY: doxygen doxygen_api doxygen_app doxy-file-target

DOXYGEN               := doxygen
DOXYGEN_OUTPUT_FOLDER := $(INSTALL_LIB_PATH)/doxygen
DOCS := $(ROOTDIR)/docs
DOXYGEN_CFG_FOLDER := $(ROOTDIR)/docs/doxygen
# Remove -include statements from $(INCLUDES), since they'll confuse Doxygen.
INCLUDES_WITHOUT_INCLUDE = $(filter-out -include %.h,$(INCLUDES))
DOXYGEN_INCLUDES      = $(patsubst -I%,%,$(INCLUDES_WITHOUT_INCLUDE))
DOXYGEN_DEFINES       = $(patsubst -D%,%,$(DEFINES))
DOXYGEN_FILES = $(ROOTDIR)/drivers/hbi/inc $(ROOTDIR)/include/ssl.h
DIR_APPL = $(ROOTDIR)
APP_FILES = $(DIR_APPL)/apps/

doxygen: doxy-file-target doxygen_api doxygen_app

# There is a bug in pdflatex that shows itself whenever a
# hyperlink is split across two pages. An example is very long
# enums (e.g. vtss_target_type_t), that are likely to get split.
# To overcome this, a page break is inserted just before the
# troublesome construct directly in the generated tex file.
# When the bug appears, "make" returns Error 2 from "make doxygen".
# Have a look at refman.log to find the page in the PDF (that doesn't get
# created) on which the error occurs. Then create the PDF by using
# the following construct in refman.tex: \usepackage[...,draft,...]{hyperref}
# and look at the generated PDF. Once found, add a rule in the
# insert_pagebreak() "function" below, that inserts \newpage
# at the beginning of the line.
# Useful links:
#   How to debug the problem:                http://tug.org/pipermail/pdftex/2002-February/002216.html
#   How to insert a page break in Tex files: http://stackoverflow.com/questions/22601053/pagebreak-in-markdown-while-creating-pdf
define insert_pagebreak
	$(Q)if [ -f $1/latex/vtss__init__api_8h.tex ]; then sed -i -e "s|^\(enum.*vtss\\\\-\\\\_\\\\-target\\\\-\\\\_\\\\-type\\\\-\\\\_\\\\-t\}.*\)|\\\\newpage \1|" $1/latex/vtss__init__api_8h.tex; fi;
endef

define doxy-doit
	$(Q)@mkdir -p $6	
	#$(Q)$(DOXYGEN) -s -g $7 >/dev/null
	#$(Q)sed -i -e "s|\(\bPROJECT_NAME\b.*=\).*|\1 \"Microsemi VPROC API\"|"    $7
	$(Q)sed -i -e "s|\(\bINPUT\b.*=\).*|\1 $2|"                        $7
	#$(Q)sed -i -e 's|\(\bPREDEFINED\b.*=\).*|\1 $3|'                   $7
	$(Q)sed -i -e 's|\(\bINCLUDE_PATH\b.*=\).*|\1 $4|'                 $7
	#$(Q)sed -i -e "s|\(\bOUTPUT_DIRECTORY\b.*=\).*|\1 $6|"             $7
	#$(Q)sed -i -e "s|\(\bFULL_PATH_NAMES\b.*=\).*|\1 YES|"             $7
	#$(Q)sed -i -e "s|\(\bSTRIP_FROM_PATH\b.*=\).*|\1 $(OBJ)/../..|"    $7
	#$(Q)sed -i -e "s|\(\bWARN_NO_PARAMDOC\b.*=\).*|\1 YES|"            $7
	#$(Q)sed -i -e "s|\(\bOPTIMIZE_OUTPUT_FOR_C\b.*=\).*|\1 YES|"       $7
	#$(Q)sed -i -e "s|\(\bSOURCE_BROWSER\b.*=\).*|\1 YES|"              $7
	#$(Q)sed -i -e "s|\(\bDISTRIBUTE_GROUP_DOC\b.*=\).*|\1 YES|"        $7
	#$(Q)sed -i -e "s|\(\bPDF_HYPERLINKS\b.*=\).*|\1 YES|"              $7
	#$(Q)sed -i -e "s|\(\bUSE_PDFLATEX\b.*=\).*|\1 YES|"                $7
	#$(Q)sed -i -e "s|\(\bLATEX_BATCHMODE\b.*=\).*|\1 YES|"             $7
	#$(Q)sed -i -e "s|\(\bEXTRACT_ALL\b.*=\).*|\1 YES|"                  $7
	#$(Q)sed -i -e "s|\(\bSORT_MEMBER_DOCS\b.*=\).*|\1 NO|"             $7
	#$(Q)sed -i -e "s|\(\bPROJECT_LOGO\b.*=\).*|\1 $(ROOTDIR)/images.png|" $7
#	$(Q)sed -i -e "s|\(\bIMAGE_PATH\b.*=\).*|\1 $(ROOTDIR)/images.png|" $7      
	#$(Q)sed -i -e "s|\(\bLATEX_EXTRA_FILES\b.*=\).*|\1 $(ROOTDIR)/images.png|" $7
	#$(Q)sed -i -e "s|\(\bLATEX_HEADER\b.*=\).*|\1 $(ROOTDIR)/header.tex|" $7  
	#$(Q)sed -i -e "s|\(\bPROJECT_BRIEF\b.*=\).*|\1 \"This document brief $5 specification for Microsemi VPROC SDK\"|" $7
	$(call what,Doxygen $1 - Processing source files)
#	$(Q)$(DOXYGEN) $7 > /dev/null
	$(Q)$(DOXYGEN) $7
	$(call what,Doxygen $1 - PDF processing of doxygen output)
	$(call insert_pagebreak,$6)
	$(Q)sed -i -- "s/latex_count=8/latex_count=20/g" $6/latex/Makefile # Seen issue with the the Makefile for Latex just exits with error code 2 (no other indication of what is wrong). Changing the latex count is the fix.
	#$(Q)$(MAKE) -C $6/latex 1>/dev/null 2>/dev/null
	$(Q)$(MAKE) -C $6/latex
	cp -f $(DOXYGEN_OUTPUT_FOLDER)/$1/latex/refman.pdf $(DOCS)/$5.pdf
endef

define doxy-target
$1:
	$$(call doxy-doit,$1,$2,$3,$4,$5,$(DOXYGEN_OUTPUT_FOLDER)/$1,$(DOXYGEN_CFG_FOLDER)/$1.cfg)
endef

# In order for doxygen to give warnings about un-documented functions, it is required that the file is doxygen documented,
# but hey, I can't figure out how to get doxygen to give a warning if a file in not doxygen documented, so therefore I
# have made the work-around below for finding files that are not documented.
define file_desc_chk
ifneq ($(strip $1),)
$$(  warning: The following files do not contain file doxygen documentation: $1)
endif

endef

doxy-file-target:
	$(eval $(call file_desc_chk,$(shell grep -L '\\file' $(ROOTDIR)/include/*.h)))
	$(eval $(call file_desc_chk,$(shell grep -L '\\file' $(ROOTDIR)/platform/$(PLATFORM)/include/*.h)))

$(eval $(call doxy-target,doxygen_api,$(DOXYGEN_FILES),$(DOXYGEN_DEFINES),$(DOXYGEN_INCLUDES),API_Specification))
$(eval $(call doxy-target,doxygen_app,$(APP_FILES),$(DOXYGEN_DEFINES),$(DIR_APPL)/apps,Apps))

