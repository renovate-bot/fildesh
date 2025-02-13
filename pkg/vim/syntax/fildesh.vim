" Vim filetype file
" Language:     Fildesh
" Author:       Alex Klinkhamer (about.fildesh@grencez.dev)
" URL:          https://github.com/fildesh/fildesh

if exists("b:current_syntax")
  finish
endif

syn case match

syn match   fildeshComment /#.*$/
syn match   fildeshInclude /\$(<< [^)]*)/
syn match   fildeshKeyword "$(barrier)"
syn match   fildeshStringEscape display contained "\\\([\"\\0tnvfr]\)"
syn region  fildeshString start=/"/ skip=/\\"/ end=/"/ contains=fildeshStringEscape
syn region  fildeshString start=/"""/ skip=/\\"/ end=/"""/ contains=fildeshStringEscape
syn region  fildeshString start=/'/ end=/'/

hi def link fildeshComment Comment
hi def link fildeshInclude Include
hi def link fildeshKeyword Keyword
hi def link fildeshString String
hi def link fildeshStringEscape SpecialChar

let b:current_syntax = "fildesh"
