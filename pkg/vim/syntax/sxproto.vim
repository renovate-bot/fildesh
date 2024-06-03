" Vim filetype file
" Language:     Sxproto
" Author:       Alex Klinkhamer (about.sxproto@grencez.dev)
" URL:          https://github.com/fildesh/fildesh

if exists("b:current_syntax")
  finish
endif

syn case match

syn match   sxprotoComment /;.*$/
syn match   sxprotoStringEscape display contained "\\\([\"\\0nrt]\)"
syn region  sxprotoString start=/"/ skip=/\\"/ end=/"/ contains=sxprotoStringEscape

hi def link sxprotoComment Comment
hi def link sxprotoInclude Include
hi def link sxprotoKeyword Keyword
hi def link sxprotoString String
hi def link sxprotoStringEscape SpecialChar

let b:current_syntax = "sxproto"
