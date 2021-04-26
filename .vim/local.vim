let g:ale_completion_enabled = 0

" let b:ale_linters = {'cpp' : [ 'cpplint', 'cppcheck' ]}

" let g:ycm_clangd_binary_path = 'clangd'
let g:ycm_clangd_args = ['--compile-commands-dir=/home/jan/workspace/open62541/build']
" imap <C-Space> <Plug>(ale_complete)
" let g:ycm_filetype_blacklist = {'cpp': 1}
" let g:ycm_filetype_whitelist = {}
" au bufenter cpp let g:ycm_auto_trigger = 0

" let b:ale_linters = {'cpp': ['clangd']}
let b:ale_linters = {'cpp': ['clangd']}
" let b:ale_linters = {'cpp': ['clangd', 'clangtidy']}
let b:ale_fixers = {'cpp': ['clang-format', 'remove_trailing_lines', 'trim_whitespace', 'uncrustify']}
" let g:ale_warn_about_trailing_whitespace = 1

"
" let g:ale_update_tagstack = 0
let g:ale_c_build_dir = '/home/jan/workspace/open62541/build'
let g:ale_fix_on_save = 1
" let g:ale_lint_on_text_changed = 'always'
" let b:ale_fixers = ['clang-format']

" let g:ale_cpp_clang_options = '-std=c++11 -Wall'
" 
" let g:ale_cpp_gcc_options = '-std=c++11 -Wall'

let g:ale_echo_msg_error_str = 'E'
let g:ale_echo_msg_warning_str = 'W'
let g:ale_echo_msg_format = '[%linter%] %s [%severity%]'
