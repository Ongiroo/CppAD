#! /bin/python
# $Id
# -----------------------------------------------------------------------------
# CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-15 Bradley M. Bell
#
# CppAD is distributed under multiple licenses. This distribution is under
# the terms of the 
#                     Eclipse Public License Version 1.0.
#
# A copy of this license is included in the COPYING file of this distribution.
# Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
# -----------------------------------------------------------------------------
# imports
from __future__ import print_function
import sys
import os
import re
import subprocess
# -----------------------------------------------------------------------------
# command line arguments
usage = '\tbin/push_git2svn.py svn_branch_path\n'
narg  = len(sys.argv)
if sys.argv[0] != 'bin/push_git2svn.py' :
	msg = 'bin/push_git2svn.py must be executed from its parent directory'
	sys.exit(usage + msg)
if narg != 2 :
	msg = 'expected 1 but found ' + str(narg-1) + ' command line arguments'
	sys.exit(usage + msg)
svn_branch_path = sys.argv[1]
if svn_branch_path == 'master' :
	msg = 'trunk is the svn_branch_path for the master branch'
	sys.exit(usage + msg)
# -----------------------------------------------------------------------------
# some settings
svn_repository = 'https://projects.coin-or.org/svn/CppAD'
git_repository = 'https://github.com/bradbell/cppad'
work_directory = 'build/push_git2svn'
# -----------------------------------------------------------------------------
# some simple functions
def pause() :
	response=''
	while response != 'y\n' and  response != 'n\n' :
		print('Continue [y/n] ')
		response = sys.stdin.readline()
	if response == 'n\n' :
		sys.exit('user abort')
#
def system(cmd) :
	try :
		output = subprocess.check_output(
			cmd,
			stderr=subprocess.STDOUT,
			shell=True
		)
	except subprocess.CalledProcessError :
		msg  = subprocess.CalledProcessError.output
		msg += '\nbin/push_git2svn.py exiting because command above failed'
		sys.exit(cmd + '\n\n' + msg)
	return output
def print_system(cmd) :
	print(cmd)
	try :
		output = subprocess.check_output(
			cmd,
			stderr=subprocess.STDOUT,
			shell=True
		)
	except subprocess.CalledProcessError as info :
		msg  = info.output
		msg += '\nbin/push_git2svn.py exiting because command above failed'
		sys.exit(msg)
	return output
# -----------------------------------------------------------------------------
# determine git_branch_path
if svn_branch_path == 'trunk' or svn_branch_path.startswith('branches/') :
	git_branch_path = svn_branch_path
else :
	git_branch_path = 'branches/' + svn_branch_path
# -----------------------------------------------------------------------------
# hash code for the git branch
if svn_branch_path == 'trunk' :
	cmd = 'git show-ref origin/master'
else :
	cmd = 'git show-ref origin/' + git_branch_path 
git_hash_code = system(cmd)
git_hash_code = git_hash_code.replace(' refs/remotes/origin/master\n', '')
# -----------------------------------------------------------------------------
# make sure work directory exists
if not os.path.isdir(work_directory) :
	os.makedirs(work_directory)
# -----------------------------------------------------------------------------
# checkout svn version of directory
svn_directory = work_directory + '/svn'
if os.path.isdir(svn_directory) :
	print('Use existing svn directory: ' + svn_directory)
	pause()
	cmd = 'svn revert --recursive ' + svn_directory
	print_system(cmd)
	cmd        = 'svn status ' + svn_directory
	svn_status = system(cmd)
	svn_status = svn_status.split('\n')
	for entry in svn_status :
		if entry.startswith('?       ') :
			file_name = entry[8:]
			cmd = 'rm ' + file_name
			system(cmd)
else :
	cmd  = 'svn checkout '
	cmd +=  svn_repository + '/' + svn_branch_path + ' ' + svn_directory
	print_system(cmd)
# ----------------------------------------------------------------------------
# git hash code corresponding to verison isn svn directory
cmd           = 'svn info ' + svn_directory
svn_info      = system(cmd)
rev_pattern   = re.compile('Last Changed Rev: ([0-9]+)')
match         = re.search(rev_pattern, svn_info)
svn_revision  = match.group(1)
cmd           = 'svn log -r ' + svn_revision + ' ' + svn_directory
svn_log       = system(cmd)
hash_pattern  = re.compile('https://github.com/bradbell/cppad ([0-9a-f]+)')
match         = re.search(hash_pattern, svn_log)
svn_hash_code = match.group(1)
# -----------------------------------------------------------------------------
# export the git verison of the directory
git_directory = work_directory + '/git'
if os.path.isdir(git_directory) :
	print('Use existing git directory: ' + git_directory)
	pause()
else :
	cmd  = 'svn export '
	cmd +=  git_repository + '/' + git_branch_path + ' ' + git_directory
	print_system(cmd)
# -----------------------------------------------------------------------------
# list of files for the svn and git directories
svn_pattern = re.compile(svn_directory + '/')
svn_file_list = []
for directory, dir_list, file_list in os.walk(svn_directory) :
	ok = ( directory.find('/.svn/') == -1 )
	ok = ok and ( not directory.endswith('/.svn') )
	if ok :
		for name in file_list :
			local_name = directory + '/' + name
			local_name = re.sub(svn_pattern, '', local_name)
			svn_file_list.append( local_name )
#
git_pattern = re.compile(git_directory + '/')
git_file_list = []
for directory, dir_list, file_list in os.walk(git_directory) :
	index =  directory.find('/.svn/')
	assert index == -1
	for name in file_list :
		local_name = directory + '/' + name
		local_name = re.sub(git_pattern, '', local_name)
		git_file_list.append( local_name )
# -----------------------------------------------------------------------------
# list of files that have been created and deleted
created_list=[]
for name in git_file_list :
	if not name in svn_file_list :
		created_list.append(name)
#
deleted_list=[]
for name in svn_file_list :
	if not name in git_file_list :
		deleted_list.append(name)

# -----------------------------------------------------------------------------
# automated svn commands
id_pattern = re.compile(r'^.*\$Id.*')
#
for git_file in created_list :
	git_f     = open(git_directory + '/' + git_file, 'rb')
	git_data  = git_f.read()
	git_f.close()
	git_data  = re.sub(id_pattern, '', git_data)
	#
	found = False
	for svn_file in deleted_list :
		svn_f    = open(svn_directory + '/' + svn_file, 'rb')
		svn_data = svn_f.read()
		svn_f.close()
		svn_data = re.sub(id_pattern, '', svn_data)
		#
		if svn_data == git_data :
			assert not found
			cmd  = 'svn copy ' + svn_directory + '/' + svn_file + ' \\\n\t'
			cmd += svn_directory + '/' + git_file
			print_system(cmd)
			found = True
	if not found :
			cmd  = 'cp ' + git_directory + '/' + git_file + ' \\\n\t'
			cmd += svn_directory + '/' + git_file 
			system(cmd)
			cmd  = 'svn add ' + svn_directory + '/' + git_file
			print_system(cmd)
#
for svn_file in deleted_list :
	svn_file_path = svn_directory + '/' + svn_file
	if os.path.isfile(svn_file_path) :
		cmd = 'svn delete --force ' + svn_file_path
		print_system(cmd)
#
for git_file in git_file_list :
	do_cp = True
	do_cp = do_cp and git_file not in created_list
	if git_file in svn_file_list :
		git_f     = open(git_directory + '/' + git_file, 'rb')
		git_data  = git_f.read()
		git_f.close()
		git_data  = re.sub(id_pattern, '', git_data)
		#
		svn_f    = open(svn_directory + '/' + git_file, 'rb')
		svn_data = svn_f.read()
		svn_f.close()
		svn_data = re.sub(id_pattern, '', svn_data)
		#
		do_cp = do_cp and git_data != svn_data
	if do_cp :
		cmd  = 'cp ' + git_directory + '/' + git_file + ' \\\n\t'
		cmd += svn_directory + '/' + git_file 
		system(cmd)
# -----------------------------------------------------------------------------
data  = 'merge to branch: ' + svn_branch_path + '\n'
data += 'from repository: ' + git_repository + '\n'
data += 'start hash code:  ' + svn_hash_code + '\n'
data += 'end   hash code:  ' + git_hash_code + '\n\n'
sed_cmd = "sed -e '/" + svn_hash_code + "/,$d'"
if svn_branch_path == 'trunk' :
	cmd    = 'git log origin/master | ' + sed_cmd
else :
	cmd    = 'git log origin/' + git_branch_path + ' | ' + sed_cmd
output = print_system(cmd)
data += output
log_f = open( svn_directory + '/push_git2svn.log' , 'wb')
log_f.write(data)
log_f.close()
msg  = '\nChange into svn directory with the command\n\t'
msg += 'cd ' + svn_directory + '\n'
msg += 'If these changes are OK, execute the command:\n\t'
msg += 'svn commit --file push_git2svn.log\n'
msg += 'You may want to edit the log push_git2svn.log'
print(msg)