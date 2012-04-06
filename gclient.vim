" gclient.vim	:	a vim script for source-line debugging in vim with gdb and gstep

let g:top_win_height = 15
"let g:help_file = g:ape_dir . "/" . "glient.vim.help.txt"
let g:help_file = "glient.vim.help.txt"

function! Init()
	echo "initializing..."
	call CloseWindows()
	call SetupWindows()
	call SetupGdb()
	call SetMappings()
endfunction

function! CloseWindows()
	"goto final window
	let endnr = winnr("$")
	while endnr > 1
		exe ":" endnr " wincmd w"
		:clo
		let endnr = winnr("$")
	endwhile
endfunction

function! SetupWindows()
	:e source
	"delete existing content
	:1,$d
	:new gdb.output
	"delete existing content
	:1,$d
endfunction

function! GotoWin(nr)
	sil exe ":" a:nr "wincmd w"
endfunction

function! GotoSrcWin()
	call GotoWin(1)
endfunction

function! GotoGdbWin()
	call GotoWin(2)
endfunction

"starts gdb and debuggee program, sets exe index pos to 0
function! SetupGdb()
	call GotoGdbWin()

	:sil exe "%!gclient start_gdb"
	:sil 1,$y r
	let result = @r

	"check gserver running
	if result == "connect() failed: Connection refused"
		:throw "can't connect to gserver"
	endif

	"check gdb running
	if result == "504 Gdb Not Running"
		:throw 'gdb not running'
	endif

	"check gdb failed to start
	if result == "503 Gdb Failed To Start"
		:throw 'gdb failed to start'
	endif

	"check gdb init response
	let exp_result = "GNU gdb"
	let result_pre = strpart(result, 0, 7)
	if result_pre != exp_result
		:throw 'gdb start result: expected: [' . exp_result . ']... . got: [' . result_pre . ']'
	endif

	"save output
	sil :w!

endfunction

"get-source-file.vim	:	parses output of gdb 'info source', returns fqp of source file
function! GetExeSourceFile()
	:sil %!gclient gdb_cmd info source
	:sil exe ":1,$y s"
	let info_source = @s
	let lines = split(info_source, "\n")
	let ln = lines[2]
	let toks = split(ln)
	let fname = toks[2]
	return fname
endfunction

"get-current-line.vim	:	parses output of gdb 'bt' command to get the current line number of a running program
"bt output:
"#0  main (argc=1, argv=0xbffff444) at main.c:450
function! GetExeSourceLine()
	:sil %!gclient gdb_cmd bt
	:sil 1,$y l
	:let g:ln = @l
	:let g:matches = matchlist(g:ln, ":\\(\\d\\+\\)")
	return g:matches[1]
endfunction

function! GotoSrcLine(src, lnum)
	call GotoSrcWin()
	:sil exe ":e " a:src
	:set cursorline
	call cursor(a:lnum, 1)
endfunction

"does a step in gdb
function! Step(step_type)

	"step
	let step_type = a:step_type
	:sil exe "%!gclient gdb_cmd " step_type

	"get line info
	let src = GetExeSourceFile()
	let lnum = GetExeSourceLine()

	"update src win
	call GotoSrcLine(src, lnum)

	call GotoGdbWin()

endfunction

function! SetMappings()
	:map <f1> :call Help()
	:map <f4> :%!gclient gdb_cmd 
	:map <f7> :call Step('step')
	:map <f8> :call Step('next')
endfunction

function! Help()
	call CloseWindows()
	exe ":e " . g:help_file
endfunction

call Init()
