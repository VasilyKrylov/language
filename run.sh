set -e

frontend/frontend rap_sources/$1.rap
backend/backend ast_forest/tree.ast

./middle_end_prog ast_forest/tree.ast 

cd ../processor

./build_and_run.sh lang_auto_compiled

cd ..