/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * DESCRIPTION
 *	Verify that mount(2) returns -1 and sets errno to  EPERM if the user
 *	is not the super-user.
 */

#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/loop.h>
#include <fcntl.h>
#include <pwd.h>
#include "test.h"
#include "safe_macros.h"

static void setup(void);
static void cleanup(void);

char *TCID = "mount04";

#define DIR_MODE	S_IRWXU | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP

static char *mntpoint = "mntpoint";
static const char fs_type[256] = "ext4";
static const char device[1024];
int loopdev_flag = 0;

int TST_TOTAL = 1;

// acquire_device: find free loop device and bind the
// filesystem image with it.
void acquire_device(void)
{
        int ctlloopdev, loopdev, fsimgfile;
        long devnum;

	// Open loop controle device file	
	ctlloopdev = open("/dev/loop-control", O_RDWR);
        if (ctlloopdev == -1)
        	tst_brkm(TBROK, cleanup, "failed to open /dev/loop-control");

	// Find the free loop device
	devnum = ioctl(ctlloopdev, LOOP_CTL_GET_FREE);
        if (devnum == -1)
        	tst_brkm(TBROK, cleanup, "failed to get free loop device");

	sprintf(device, "/dev/loop%ld", devnum);
	tst_resm(TINFO, "device = %s\n", device);

	// Open a loop device
	loopdev = open(device, O_RDWR);
        if (loopdev == -1)
        	tst_brkm(TBROK, cleanup, "failed to open: loop device");

	// open a filesystem image
	fsimgfile = open("/ltp_tst_mnt_fs/tstfs_ext4.img", O_RDWR);
        if (fsimgfile == -1)
        	tst_brkm(TBROK, cleanup, "failed to open: file system image file");

	// bind filesystem image loop device
        if (ioctl(loopdev, LOOP_SET_FD, fsimgfile) == -1)
               tst_brkm(TBROK, cleanup, "failed to set loop fd");

	close(fsimgfile);
    	close(loopdev);
	loopdev_flag = 1;
}

static void verify_mount(void)
{

	TEST(mount(device, mntpoint, fs_type, 0, NULL));

	if (TEST_RETURN == -1) {
		if (TEST_ERRNO == EPERM)
			tst_resm(TPASS | TTERRNO, "mount() failed expectedly");
		else
			tst_resm(TFAIL | TTERRNO,
			         "mount() was expected to fail with EPERM");
		return;
	}

	if (TEST_RETURN == 0) {
		tst_resm(TFAIL, "mount() succeeded unexpectedly");
		if (tst_umount(mntpoint))
			tst_brkm(TBROK, cleanup, "umount() failed");
		return;
	}

	tst_resm(TFAIL | TTERRNO, "mount() returned %li", TEST_RETURN);
}

int main(int ac, char **av)
{
	int lc;

	tst_parse_opts(ac, av, NULL, NULL);

	setup();

	for (lc = 0; TEST_LOOPING(lc); lc++) {
		tst_count = 0;
		verify_mount();
	}

	cleanup();
	tst_exit();
}

static void setup(void)
{
	char nobody_uid[] = "nobody";
	struct passwd *ltpuser;

	tst_sig(FORK, DEF_HANDLER, cleanup);

	tst_require_root();

	tst_tmpdir();

// Framework code causing the oom killer issue
// Hence, Removed the code.
#if 0
	fs_type = tst_dev_fs_type();
	device = tst_acquire_device(cleanup);

	if (!device)
		tst_brkm(TCONF, cleanup, "Failed to obtain block device");

	tst_mkfs(cleanup, device, fs_type, NULL, NULL);
#endif
	
	acquire_device();

	ltpuser = SAFE_GETPWNAM(cleanup, nobody_uid);
	SAFE_SETEUID(cleanup, ltpuser->pw_uid);
	SAFE_MKDIR(cleanup, mntpoint, DIR_MODE);

	TEST_PAUSE;
}

static void cleanup(void)
{
	if (seteuid(0))
		tst_resm(TWARN | TERRNO, "seteuid(0) failed");

// Framework code causing the oom killer issue
// Hence, Removed the code.
#if 0
	if (device)
		tst_release_device(device);
#endif
	if (loopdev_flag == 1) {
		int devfd = open(device, O_RDWR);
		if (ioctl(devfd, LOOP_CLR_FD, 0) < 0) {
    			tst_resm(TWARN | TERRNO, "Failed to clear fd");
		}
		close(devfd);
	}

	tst_rmdir();
}
