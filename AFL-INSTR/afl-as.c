#include "afl-as.h"

static u8*  input_file;         /* Originally specified input file      */
static u8*  modified_file;      /* Instrumented file for the real 'as'  */
static u8 inst_ratio = 100;   /* Instrumentation probability (%)      */

static u8   be_quiet,           /* Quiet mode (no stderr output)        */
       clang_mode,         /* Running in clang mode?               */
       pass_thru,          /* Just pass data through?              */
       just_version,       /* Just show version?                   */
       sanitizer;          /* Using ASAN / MSAN                    */


static void add_instrumentation(void) {

	static u8 line[MAX_LINE];

	FILE* inf;
	FILE* outf;
	s32 outfd;
	u32 ins_lines = 0;

	u8  instr_ok = 0, skip_csect = 0, skip_next_label = 0,
	    skip_intel = 0, skip_app = 0, instrument_next = 0;

#ifdef __APPLE__

	u8* colon_pos;

#endif /* __APPLE__ */

	if (input_file) {

		inf = fopen(input_file, "r");
		if (!inf) PFATAL("Unable to read '%s'", input_file);

	} else inf = stdin;

	outfd = open(modified_file, O_WRONLY | O_EXCL | O_CREAT, 0600);

	if (outfd < 0) PFATAL("Unable to write to '%s': %d", modified_file, outfd);

	outf = fdopen(outfd, "w");

	if (!outf) PFATAL("fdopen() failed");

	while (fgets(line, MAX_LINE, inf)) {

		/* In some cases, we want to defer writing the instrumentation trampoline
		   until after all the labels, macros, comments, etc. If we're in this
		   mode, and if the line starts with a tab followed by a character, dump
		   the trampoline now. */

		if (!pass_thru && !skip_intel && !skip_app && !skip_csect && instr_ok &&
		    instrument_next && line[0] == '\t' && isalpha(line[1])) {

			fprintf(outf, use_64bit ? trampoline_fmt_64 : trampoline_fmt_32,
			        R(MAP_SIZE));

			instrument_next = 0;
			ins_lines++;

		}

		/* Output the actual line, call it a day in pass-thru mode. */

		fputs(line, outf);

		if (pass_thru) continue;

		/* All right, this is where the actual fun begins. For one, we only want to
		   instrument the .text section. So, let's keep track of that in processed
		   files - and let's set instr_ok accordingly. */

		if (line[0] == '\t' && line[1] == '.') {

			/* OpenBSD puts jump tables directly inline with the code, which is
			   a bit annoying. They use a specific format of p2align directives
			   around them, so we use that as a signal. */

			if (!clang_mode && instr_ok && !strncmp(line + 2, "p2align ", 8) &&
			    isdigit(line[10]) && line[11] == '\n') skip_next_label = 1;

			if (!strncmp(line + 2, "text\n", 5) ||
			    !strncmp(line + 2, "section\t.text", 13) ||
			    !strncmp(line + 2, "section\t__TEXT,__text", 21) ||
			    !strncmp(line + 2, "section __TEXT,__text", 21)) {
				instr_ok = 1;
				continue;
			}

			if (!strncmp(line + 2, "section\t", 8) ||
			    !strncmp(line + 2, "section ", 8) ||
			    !strncmp(line + 2, "bss\n", 4) ||
			    !strncmp(line + 2, "data\n", 5)) {
				instr_ok = 0;
				continue;
			}

		}

		/* Detect off-flavor assembly (rare, happens in gdb). When this is
		   encountered, we set skip_csect until the opposite directive is
		   seen, and we do not instrument. */

		if (strstr(line, ".code")) {

			if (strstr(line, ".code32")) skip_csect = use_64bit;
			if (strstr(line, ".code64")) skip_csect = !use_64bit;

		}

		/* Detect syntax changes, as could happen with hand-written assembly.
		   Skip Intel blocks, resume instrumentation when back to AT&T. */

		if (strstr(line, ".intel_syntax")) skip_intel = 1;
		if (strstr(line, ".att_syntax")) skip_intel = 0;

		/* Detect and skip ad-hoc __asm__ blocks, likewise skipping them. */

		if (line[0] == '#' || line[1] == '#') {

			if (strstr(line, "#APP")) skip_app = 1;
			if (strstr(line, "#NO_APP")) skip_app = 0;

		}

		/* If we're in the right mood for instrumenting, check for function
		   names or conditional labels. This is a bit messy, but in essence,
		   we want to catch:

		     ^main:      - function entry point (always instrumented)
		     ^.L0:       - GCC branch label
		     ^.LBB0_0:   - clang branch label (but only in clang mode)
		     ^\tjnz foo  - conditional branches

		   ...but not:

		     ^# BB#0:    - clang comments
		     ^ # BB#0:   - ditto
		     ^.Ltmp0:    - clang non-branch labels
		     ^.LC0       - GCC non-branch labels
		     ^.LBB0_0:   - ditto (when in GCC mode)
		     ^\tjmp foo  - non-conditional jumps

		   Additionally, clang and GCC on MacOS X follow a different convention
		   with no leading dots on labels, hence the weird maze of #ifdefs
		   later on.

		 */

		if (skip_intel || skip_app || skip_csect || !instr_ok ||
		    line[0] == '#' || line[0] == ' ') continue;

		/* Conditional branch instruction (jnz, etc). We append the instrumentation
		   right after the branch (to instrument the not-taken path) and at the
		   branch destination label (handled later on). */

		if (line[0] == '\t') {

			if (line[1] == 'j' && line[2] != 'm' && R(100) < inst_ratio) {

				fprintf(outf, use_64bit ? trampoline_fmt_64 : trampoline_fmt_32,
				        R(MAP_SIZE));

				ins_lines++;

			}

			continue;

		}

		/* Label of some sort. This may be a branch destination, but we need to
		   tread carefully and account for several different formatting
		   conventions. */

#ifdef __APPLE__

		/* Apple: L<whatever><digit>: */

		if ((colon_pos = strstr(line, ":"))) {

			if (line[0] == 'L' && isdigit(*(colon_pos - 1))) {

#else

		/* Everybody else: .L<whatever>: */

		if (strstr(line, ":")) {

			if (line[0] == '.') {

#endif /* __APPLE__ */

				/* .L0: or LBB0_0: style jump destination */

#ifdef __APPLE__

				/* Apple: L<num> / LBB<num> */

				if ((isdigit(line[1]) || (clang_mode && !strncmp(line, "LBB", 3)))
				    && R(100) < inst_ratio) {

#else

				/* Apple: .L<num> / .LBB<num> */

				if ((isdigit(line[2]) || (clang_mode && !strncmp(line + 1, "LBB", 3)))
				    && R(100) < inst_ratio) {

#endif /* __APPLE__ */

					/* An optimization is possible here by adding the code only if the
					   label is mentioned in the code in contexts other than call / jmp.
					   That said, this complicates the code by requiring two-pass
					   processing (messy with stdin), and results in a speed gain
					   typically under 10%, because compilers are generally pretty good
					   about not generating spurious intra-function jumps.

					   We use deferred output chiefly to avoid disrupting
					   .Lfunc_begin0-style exception handling calculations (a problem on
					   MacOS X). */

					if (!skip_next_label) instrument_next = 1; else skip_next_label = 0;

				}

			} else {

				/* Function label (always instrumented, deferred mode). */

				instrument_next = 1;

			}

		}

	}

	if (ins_lines)
		fputs(use_64bit ? main_payload_64 : main_payload_32, outf);

	if (input_file) fclose(inf);
	fclose(outf);

	if (!be_quiet) {

		if (!ins_lines) WARNF("No instrumentation targets found%s.",
			                      pass_thru ? " (pass-thru mode)" : "");
		else OKF("Instrumented %u locations (%s-bit, %s mode, ratio %u%%).",
			         ins_lines, use_64bit ? "64" : "32",
			         getenv("AFL_HARDEN") ? "hardened" :
			         (sanitizer ? "ASAN/MSAN" : "non-hardened"),
			         inst_ratio);

	}

}

int main(int argc, char** argv) {
	input_file = argv[argc - 2];
	modified_file = argv[argc - 1];
	printf("input_file: %s\nmodified_file: %s", input_file, modified_file);

	add_instrumentation();
	return 0;
}