#include "pix-internal.h"

static pix_env_t init_process;
pix_env_t *pix_cur_process = &init_process;


// create a new pix structure that fills in the 
// register save area with the right initial values so 
// that when you use <switch_to> on it for the first time
// it will behave identically to calling <user_mode_run_fn>
pix_env_t pix_env_mk(uint32_t pc, uint32_t sp) {
    pix_env_t proc = {0};
	proc.reg_save[0] =  pc;
	proc.reg_save[1] = sp;
	proc.reg_save[15] = pc;
	proc.reg_save[13] = sp;
	proc.reg_save[16] = 0x190;
    return proc;
}
