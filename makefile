MODEL_PATH=apps/tools/normgraph

FIRM_LOG_LEVEL=LOG_WARN
PROC_LOG_LEVEL=LOG_INFO
CC=clang++
CFLAGS += -I. -Iapps -Iimpl -I./ -Iexps  -O3 -std=c++20 ${LOG_LEVEL} -DNDEBUG 

# Object files
FORMAT_OBJ=${MODEL_PATH}/format.o
DIVIDE_OBJ=${MODEL_PATH}/divide.o
PROCESS_OBJ=${MODEL_PATH}/process.o
EXP_QUERY_OBJ=exps/query_exp
EXP_UPDATE_OBJ=exps/update_exp

all: format divide process exp_query build_time alpha_update  edge_update

%.o: %.cpp %.hpp
	${CC} -c $< -o $@ $(CFLAGS)

%.o: %.cpp
	${CC} -c $< -o $@ $(CFLAGS)

format: $(FORMAT_OBJ)
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

divide: $(DIVIDE_OBJ)
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

process: $(PROCESS_OBJ)
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

build_time: $(EXP_QUERY_OBJ)/build_time.o
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

exp_query: $(EXP_QUERY_OBJ)/query_exp.o
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

alpha_update: $(EXP_UPDATE_OBJ)/alpha_update.o
	${CC} ${CFLAGS} -pg -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

edge_update: $(EXP_UPDATE_OBJ)/edge_update.o
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

clean:
	rm -f demo_run firm format divide process build_time exp_query edge_update alpha_update *.o exps/query_exp/*.o exps/update_exp/*.o ${MODEL_PATH}/*.o

.PHONY: clean