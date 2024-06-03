" Vim indent file
" Language:     Sxproto
" Author:       Alex Klinkhamer (about.sxproto@grencez.dev)
" URL:          https://github.com/fildesh/fildesh

" Only load this indent file when no other was loaded.
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetSxprotoIndent()
setlocal indentkeys=0),!^F,o,O,e

" Only define the function once.
if exists("*GetSxprotoIndent")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim


function GetSxprotoIndent()
  " Get current line
  let line = getline(v:lnum)
  if v:lnum == 0
    return 0
  endif

  let pnum = prevnonblank(v:lnum-1)
  if pnum == 0
    return 0
  endif

  let paren_count = 0
  for c in split(line, '\zs')
    if c == ')'
      let paren_count = paren_count + 1
    elseif c != ' '
      break
    endif
  endfor

  let bnum = pnum
  let parencol = 0

  while bnum > 0
    let bline = getline(bnum)
    if bline =~ '^\s*;.*$'
      let bline = ''
    endif
    let col = strlen(bline)
    for c in reverse(split(bline, '\zs'))
      if c == '('
        if paren_count == 0
          let parencol = col
          break
        endif
        let paren_count = paren_count - 1
      elseif c == ')'
        let paren_count = paren_count + 1
      endif
      let col = col - 1
    endfor
    unlet col

    if parencol > 0
      break
    endif
    if paren_count == 0
      return indent(bnum)
    endif

    let bnum = prevnonblank(bnum - 1)
  endwhile
  unlet paren_count

  if parencol > 0
    return parencol
  endif

  return indent(pnum)
endfunction


let &cpo = s:cpo_save
unlet s:cpo_save
