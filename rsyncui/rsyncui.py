#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from tkinter import *
import tkinter.ttk
from tkinter.messagebox import askyesno, showerror, showinfo
from tkinter.filedialog import asksaveasfilename, askopenfilename, askdirectory
from tkinter.scrolledtext import ScrolledText

import sys
import os, signal
import subprocess, _thread
import queue
import time
import math
import pickle
import logging

########################################################################
# commons, make weiget
########################################################################
def makeFormRow(parent, label, width=12, browse=True):
    var = StringVar()

    row = Frame(parent)
    row.config(padx=15, pady=5)
    lab = Label(
        row, text=label, width=width, font=('Helvetica', 12), relief=FLAT)
    ent = Entry(row, relief=SUNKEN, textvariable=var, font=('Helvetica', 12))
    row.pack(fill=X)
    lab.pack(side=LEFT)
    ent.pack(side=LEFT, expand=YES, fill=X)

    if browse:
        btn = Button(
            row,
            text='浏览...',
            font=('Helvetica', 12),
            padx=10,
            pady=2,
            relief=FLAT)
        btn.pack(side=RIGHT)
        btn.config(command=lambda: var.set(askdirectory()))

    return var


def makeInfoRow(parent, name, width=10):
    row = Frame(parent)
    row.config(padx=10, pady=5)
    name = Label(
        row, text=name, width=width, font=('Helvetica', 12), relief=FLAT)
    info = Label(
        row, text='尚未开始', relief=FLAT, anchor='w', font=('Helvetica', 12))

    row.pack(fill=X)
    name.pack(side=LEFT)
    info.pack(fill=X)

    return info


def makePgsBar(parent):
    frame = Frame(parent, padx=15, pady=5)
    pgs_bar = tkinter.ttk.Progressbar(
        frame, maximum=100, mode='determinate', value=0)
    number = Label(frame, text='0%', width=8, font=('Helvetica', 12))
    number.pack(side=RIGHT)
    frame.pack(fill=X)
    pgs_bar.pack(fill=X)
    return pgs_bar, number


def makeCheckButton(parent, text, **args):
    var = IntVar()
    var.set(0)
    Checkbutton(
        parent, text=text, variable=var, font=('Helvetica', 12), **args).pack(
            side=LEFT)
    return var


def sec2str(allTime):
    day = 24 * 60 * 60
    hour = 60 * 60
    min = 60
    if allTime < 60:
        return "%d s" % math.ceil(allTime)
    elif allTime > day:
        days = divmod(allTime, day)
        return "%d d, %s" % (int(days[0]), sec2str(days[1]))
    elif allTime > hour:
        hours = divmod(allTime, hour)
        return '%d h, %s' % (int(hours[0]), sec2str(hours[1]))
    else:
        mins = divmod(allTime, min)
        return "%d m, %d s" % (int(mins[0]), math.ceil(mins[1]))






########################################################################
# build rsync command
########################################################################
class Command_builder():
    def __init__(self, superuser):
        self.component_list = ['rsync', '-h', '--progress']
        if superuser:
            self.component_list.insert(0, 'pkexec')
            #self.component_list = ['sudo','-p',"''",'-S','rsync','-h','-r','--progress']

    def add_opt(self, opt):
        self.component_list.append(opt)

    def generate_command(self):
        return ' '.join(self.component_list)




########################################################################
# main application
########################################################################
class Application():

    OPT_FRAME = 0
    PGS_FRAME = 1
    ERR_FRAME = 2

    def __init__(self):

        self.root = Tk()
        self.root.config()
        self.root.geometry('850x450')
        self.root.title('rsync ui')
        self.root.protocol('WM_DELETE_WINDOW', self.onQuit)
        self.defautlbg = self.root.cget('bg')

        self.make_navigation_bar()
        self.cur_tab = self.OPT_FRAME
        self.opt_frame = self.makeframe_opt()
        self.pgs_frame = self.makeframe_pgs()
        self.err_frame = self.makeframe_err()

        # is rsync is running
        self.isrunning = 0
        # have any fail when copy
        self.haveerror = 0

        self.root.mainloop()

    def make_navigation_bar(self):
        tabs = Frame(self.root, padx=10, pady=2)
        self.opt_btn = Button(
            tabs,
            text='选项',
            command=self.onChangetabs_opt,
            font=('Helvetica', 12),
            relief=FLAT)
        self.opt_btn.pack(side=LEFT)
        self.pgs_btn = Button(
            tabs,
            text='进度',
            command=self.onChangetabs_pgs,
            font=('Helvetica', 12),
            relief=FLAT,
            bg='LightCyan')
        self.pgs_btn.pack(side=LEFT)
        self.err_btn = Button(
            tabs,
            text='错误(0)',
            command=self.onChangetabs_err,
            font=('Helvetica', 12),
            relief=FLAT,
            bg='Azure')
        self.err_btn.pack(side=LEFT)

        helpbtn = Menubutton(
            tabs, text='帮助', font=('Helvetica', 12), underline=0, relief=FLAT)
        helpbtn.pack(side=RIGHT)

        help = Menu(helpbtn)
        help.add_command(
            label='关于', command=self.about, font=('Helvetica', 12))
        help.add_command(
            label='rsync 版本', command=self.rsync_info, font=('Helvetica', 12))
        help.add_command(
            label='rsync 手册', command=self.rsync_man, font=('Helvetica', 12))
        helpbtn.config(menu=help)

        tabs.pack(side=TOP, fill=X)

    def makeframe_opt(self):
        opt_frame = Frame(self.root)

        self.opt_src = makeFormRow(opt_frame, '源文件夹')
        self.opt_dest = makeFormRow(opt_frame, '目标文件夹')


        btn_frame = Frame(opt_frame, padx=15, pady=10)
        Button(
            btn_frame,
            text='加载配置',
            command=self.load_config,
            font=('Helvetica', 12)).pack(side=LEFT)
        Button(
            btn_frame,
            text='保存配置',
            command=self.stor_config,
            font=('Helvetica', 12)).pack(side=LEFT)
        Button(
            btn_frame,
            text='查看命令',
            command=self.show_command,
            font=('Helvetica', 12)).pack(sid=RIGHT)
        self.start_bt = Button(
            btn_frame, text='开始', command=self.start, font=('Helvetica', 12))
        self.start_bt.pack(side=RIGHT)
        btn_frame.pack(fill=X, side=BOTTOM)


        center_frame = Frame(opt_frame)

        opts_container = Frame(center_frame)

        checkbutton_fram1 = Frame(opts_container)
        checkbutton_fram1.config(padx=15, pady=10)
        self.opt_recursive = makeCheckButton(checkbutton_fram1, '递归复制子文件')
        self.opt_recursive.set(1)
        self.opt_preserve_time = makeCheckButton(checkbutton_fram1, '保留时间')
        self.opt_preserve_permission = makeCheckButton(checkbutton_fram1,'保留权限')
        self.opt_preserve_owner = None
        self.opt_preserve_owner = makeCheckButton(checkbutton_fram1, '保留所属者')
        self.opt_preserve_group = makeCheckButton(checkbutton_fram1, '保留所属组')
        checkbutton_fram1.pack(fill=X)

        checkbutton_fram2 = Frame(opts_container)
        checkbutton_fram2.config(padx=15, pady=10)
        self.opt_preserve_hardlink = makeCheckButton(checkbutton_fram2,'保留硬连接')
        self.opt_preserve_symlink = makeCheckButton(checkbutton_fram2, '保留软连接')
        self.opt_preserve_device = makeCheckButton(checkbutton_fram2, '保留设备文件')
        checkbutton_fram2.pack(fill=X)

        checkbutton_fram3 = Frame(opts_container)
        checkbutton_fram3.config(padx=15, pady=10)
        self.opt_delete = makeCheckButton(checkbutton_fram3, '删除不在源中的文件')
        self.opt_skip_exist = makeCheckButton(checkbutton_fram3, '跳过已存在的文件')
        checkbutton_fram3.pack(fill=X)

        checkbutton_fram4 = Frame(opts_container)
        checkbutton_fram4.config(padx=15, pady=10)
        self.opt_super_user = makeCheckButton(checkbutton_fram4, '以超级用户运行')
        self.opt_non_incre = makeCheckButton(checkbutton_fram4, '开始复制前完成扫描')
        self.opt_non_incre.set(1)
        checkbutton_fram4.pack(fill=X)  

        self.opt_additional = makeFormRow(opts_container, '附加选项', 12, False)

        opts_container.pack(side=LEFT)

        description_container = Frame(center_frame)
        description_container.config(padx=10,pady=5)
        Label(description_container, text="描述", font=('Helvetica', 12), relief=FLAT).pack(side=TOP)
        self.opt_description = ScrolledText(description_container, font=('Helvetica', 12))
        self.opt_description.pack(expand=True,fill=BOTH)
        description_container.pack(expand=True,fill=BOTH)

        center_frame.pack(fill=BOTH,expand=True)



        opt_frame.pack(expand=True, fill=BOTH)

        return opt_frame

    def makeframe_pgs(self):
        pgs_frame = Frame(self.root)

        self.pgs_operate_label = makeInfoRow(pgs_frame, '当前操作')
        self.pgs_cur_pgs_bar, self.pgs_cur_pgs_percent = makePgsBar(pgs_frame)

        curinfo_frame = Frame(pgs_frame)
        curinfo_frame.pack(fill=X)
        self.pgs_curinfo_label = Label(
            curinfo_frame,
            text='0k      0k/s',
            width=30,
            font=('Helvetica', 12))
        self.pgs_curinfo_label.pack(side=RIGHT)

        self.pgs_total_pgs_label = makeInfoRow(pgs_frame, '总进度')
        self.pgs_total_pgs_bar, self.pgs_total_pgs_percent = makePgsBar(
            pgs_frame)

        timeleft_frame = Frame(pgs_frame)
        timeleft_frame.pack(fill=X)
        self.pgs_timeleft_label = Label(
            timeleft_frame, text='剩余时间   0', width=30, font=('Helvetica', 12))
        self.pgs_timeleft_label.pack(side=RIGHT)

        btn_frame = Frame(pgs_frame)
        btn_frame.config(padx=15, pady=10)
        btn_frame.pack(side=BOTTOM, fill=X)
        self.stop_bt = Button(
            btn_frame,
            text='终止rsync',
            command=self.onStop,
            state=DISABLED,
            font=('Helvetica', 12))
        self.stop_bt.pack(side=LEFT)
        Button(
            btn_frame,
            text='保存输出',
            command=self.saveout,
            font=('Helvetica', 12)).pack(side=RIGHT)

        stframe = Frame(pgs_frame, padx=10, pady=5)
        self.out_st = ScrolledText(stframe, font=('Helvetica', 12))
        stframe.pack(expand=True, fill=BOTH)
        self.out_st.pack(expand=True, fill=BOTH)

        return pgs_frame

    def makeframe_err(self):
        err_frame = Frame(self.root)
        err_frame.config(padx=10, pady=5)

        errbtn_frame = Frame(err_frame)
        errbtn_frame.config(padx=5, pady=5)
        errbtn_frame.pack(side=BOTTOM, fill=X)
        Button(
            errbtn_frame,
            text='保存',
            command=self.saveerr,
            font=('Helvetica', 12)).pack(side=RIGHT)

        self.err_st = ScrolledText(err_frame, font=('Helvetica', 12))
        self.err_st.pack(expand=True, fill=BOTH, side=TOP)

        return err_frame

    def onChangetabs_opt(self):
        if self.cur_tab == self.PGS_FRAME:
            self.pgs_frame.pack_forget()
            self.pgs_btn.config(bg='LightCyan')
            self.opt_frame.pack(expand=True, fill=BOTH)
        elif self.cur_tab == self.ERR_FRAME:
            self.err_frame.pack_forget()
            self.err_btn.config(bg='Azure')
            self.opt_frame.pack(expand=True, fill=BOTH)
        self.cur_tab = self.OPT_FRAME
        self.opt_btn.config(bg=self.defautlbg)

    def onChangetabs_pgs(self):
        if self.cur_tab == self.OPT_FRAME:
            self.opt_frame.pack_forget()
            self.opt_btn.config(bg='Lavender')
            self.pgs_frame.pack(expand=True, fill=BOTH)
        elif self.cur_tab == self.ERR_FRAME:
            self.err_frame.pack_forget()
            self.err_btn.config(bg='Azure')
            self.pgs_frame.pack(expand=True, fill=BOTH)
        self.cur_tab = self.PGS_FRAME
        self.pgs_btn.config(bg=self.defautlbg)

    def onChangetabs_err(self):
        if self.cur_tab == self.OPT_FRAME:
            self.opt_frame.pack_forget()
            self.opt_btn.config(bg='Lavender')
            self.err_frame.pack(expand=True, fill=BOTH)
        elif self.cur_tab == self.PGS_FRAME:
            self.pgs_frame.pack_forget()
            self.pgs_btn.config(bg='LightCyan')
            self.err_frame.pack(expand=True, fill=BOTH)
        self.cur_tab = self.ERR_FRAME
        self.err_btn.config(bg=self.defautlbg)

    def start(self):
        if not self.opt_src.get():
            showerror('Rysnc Gui', '源文件路径不能为空！')
            return
        if not self.opt_dest.get():
            showerror('Rsync Gui', '目标路径不能为空！')
            return

        command = self.get_command()

        # when stop, we wait both out pipe and error pipe close
        # so we can get all output and errmessage
        self.isrunning = 2
        self.haveerror = 0

        self.start_bt.config(state=DISABLED)
        self.stop_bt.config(state=NORMAL)

        self.err_btn.config(text='错误(0)')
        self.pgs_operate_label.config(fg='black')
        self.onChangetabs_pgs()
        self.out_st.delete('1.0', END)
        self.err_st.delete('1.0', END)

        self.CUROPTIONS = 0
        self.CURPROGRESS = 1
        self.CURAMOUNT = 2
        self.CURSPEED = 3
        self.TOTAPROGRESS = 4
        self.TOTALNUMBER = 5
        #self.datalist = ['CurOptions','CurPgs','CurMount','CurSpeed','TotalPgs','TotalNumber']
        self.datalist = ['', 0, '', '', 0, '']

        self.errQueue = queue.Queue()

        self.outtext = 'Start at: ' + time.ctime(
        ) + '\n' + 'Command:  ' + command + '\n'
        self.outstsize = 0

        self.start_time = time.time()

        self.pipe = subprocess.Popen(
            command,
            bufsize=0,
            # universal_newlines is important
            # rsync may output a \t during copy a big file
            # if universal_newlines is not set to True, we will get nothing untill the copy finish.
            universal_newlines=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            #stdin=subprocess.PIPE,
            shell=True,
            # more than one subprocess will start
            # to kill all when stop, we need this
            preexec_fn=os.setsid)
        #self.pipe.stdin.write(passwd+'\n')

        # To avoid blocking ui we handle output and error in thread
        # and call fresh_ui periodically
        _thread.start_new_thread(self.read_out, ())
        _thread.start_new_thread(self.read_err, ())
        self.fresh_ui()

    def stop(self):
        logging.info('self.stop called!')
        if self.isrunning:
            os.killpg(self.pipe.pid, signal.SIGTERM)
            self.start_bt.config(state=NORMAL)
            self.stop_bt.config(state=DISABLED)

    def onStop(self):
        if askyesno('rsync ui', '停止执行？'):
            self.stop()

    def onQuit(self):
        if not self.isrunning:
            self.root.quit()
        else:
            if askyesno('Rsync_GUI', 'rsync 命令正在执行！强制退出？'):
                logging.warning('force quit')
                self.stop()
                self.root.quit()

    def read_out(self):
        while True:
            line = self.pipe.stdout.readline()
            if not line:
                break

            l = line.strip().split()
            if len(l) == 4 and l[1][-1] == ('%') and l[2].endswith('/s'):
                self.datalist[self.CURPROGRESS] = l[1][:-1]
                self.datalist[self.CURSPEED] = l[2]
                self.datalist[self.CURAMOUNT] = l[0]
            elif len(l) == 6 and l[1][-1] == '%' and l[4].startswith('(xfr#'):
                self.datalist[self.CURAMOUNT] = l[0]
                self.datalist[self.CURPROGRESS] = l[1][:-1]
                self.datalist[self.CURSPEED] = l[2]
                rate = l[5][:-1].split('=')[-1].split('/')
                self.datalist[self.TOTAPROGRESS] = (
                    float(rate[1]) - float(rate[0])) / float(rate[1]) * 100
                filenum = l[4][:-1].split('#')[-1]
                self.datalist[self.TOTALNUMBER] = str(filenum) + '/' + str(
                    int(rate[1]) - int(rate[0])) + '/' + str(rate[1])
            else:
                if not line.strip():
                    continue
                self.datalist[self.CUROPTIONS] = line.strip()
                self.outtext += line
        self.isrunning -= 1
        logging.info('pipe stdout close')

    def read_err(self):
        while True:
            line = self.pipe.stderr.readline()
            if not line:
                break
            logging.warning('something wrong when run command: %s',
                            line.strip())
            self.errQueue.put(line)
        self.isrunning -= 1
        logging.info('pipe stderr close')

    def fresh_ui(self, oneTime=False):
        while not self.errQueue.empty():
            line = self.errQueue.get()
            if not line.strip():
                continue

            self.haveerror += 1
            self.err_st.insert(END, line)
            self.err_st.see(END)

        self.err_btn.config(text='错误(%d)' % self.haveerror)

        try:
            self.pgs_operate_label.config(text=self.datalist[self.CUROPTIONS])
            self.pgs_cur_pgs_bar.config(value=self.datalist[self.CURPROGRESS])
            self.pgs_cur_pgs_percent.config(
                text=str(self.datalist[self.CURPROGRESS]) + '%')
            self.pgs_curinfo_label.config(
                text='已复制 ' + self.datalist[self.CURAMOUNT] + '     速度 ' +
                self.datalist[self.CURSPEED])
            self.pgs_total_pgs_bar.config(
                value=self.datalist[self.TOTAPROGRESS])
            self.pgs_total_pgs_percent.config(
                text=('%.2f' % self.datalist[self.TOTAPROGRESS]) + '%')
            self.pgs_total_pgs_label.config(
                text=self.datalist[self.TOTALNUMBER])
            if self.datalist[self.TOTAPROGRESS]:
                timeuse = time.time() - self.start_time
                self.pgs_timeleft_label.config(
                    text='剩余时间 ' +
                    sec2str(timeuse * 100 / self.datalist[self.TOTAPROGRESS] -
                            timeuse))

            curtotalsize = len(self.outtext)
            self.out_st.insert(END, self.outtext[self.outstsize:curtotalsize])
            self.out_st.see(END)
            self.outstsize = curtotalsize
        except:
            logging.error('fail to fresh ui for unknow command output!')

        if not oneTime:
            if self.isrunning:
                self.pgs_frame.after(500, lambda: self.fresh_ui())
            else:
                self.fresh_ui(oneTime=True)
                self.afterfinish()

    def afterfinish(self):
        self.start_bt.config(state=NORMAL)
        self.stop_bt.config(state=DISABLED)
        if self.haveerror:
            self.pgs_operate_label.config(
                text='执行结束，但命令被终止或有文件复制失败!', fg='red')
        else:
            self.pgs_operate_label.config(text='执行成功，未发生错误', fg='green')

    def show_command(self):
        showinfo('rsync command', self.get_command())

    def get_command(self):
        su = True if self.opt_super_user.get() else False
        cb = Command_builder(su)

        if self.opt_recursive.get():
            cb.add_opt('-r')
        if self.opt_preserve_time.get():
            cb.add_opt('-t')
        if self.opt_preserve_owner.get():
            cb.add_opt('-o')
        if self.opt_preserve_group.get():
            cb.add_opt('-g')
        if self.opt_preserve_permission.get():
            cb.add_opt('-p')
        if self.opt_preserve_hardlink.get():
            cb.add_opt('-H')
        if self.opt_preserve_symlink.get():
            cb.add_opt('-l')
        if self.opt_preserve_device.get():
            cb.add_opt('-D')
        if self.opt_delete.get():
            cb.add_opt('--delete')
        if self.opt_skip_exist.get():
            cb.add_opt('--ignore-existing')
        if self.opt_non_incre.get():
            cb.add_opt('--no-i-r')
        if self.opt_additional.get().strip():
            cb.add_opt(self.opt_additional.get().strip())

        cb.add_opt(self.opt_src.get())
        cb.add_opt(self.opt_dest.get())
        command = cb.generate_command()
        logging.info('rsync command is %s', command)
        return command

    def saveout(self):
        savename = asksaveasfilename()
        if savename:
            text = self.out_st.get('1.0', END + '-1c')
            try:
                file = open(savename, 'w')
                file.write(text)
                file.close()
            except:
                showerror('Rsync Gui', '写入文件失败' + savename)
            else:
                showinfo('Rsync Gui', '保存成功!')

    def saveerr(self):
        savename = asksaveasfilename()
        if savename:
            text = self.err_st.get('1.0', END + '-1c')
            try:
                file = open(savename, 'w')
                file.write(text)
                file.close()
            except:
                showerror('Rsync Gui', '写入文件失败' + savename)
            else:
                showinfo('Rsync Gui', '保存成功！')

    def load_config(self):
        readname = askopenfilename(filetypes=[('Config file', '*.pkl')])
        if not readname:
            return

        try:
            file = open(readname, 'rb')
            configdir = pickle.load(file)
            file.close()
            logging.info('load config from ' + readname)
        except:
            showerror('Rsync Gui', '无法写入文件' + savename)
        else:
            try:
                self.opt_super_user.set(configdir['super_user'])
                self.opt_recursive.set(configdir['recursive'])
                self.opt_preserve_time.set(configdir['preserve_time'])
                self.opt_preserve_owner.set(configdir['preserve_owner'])
                self.opt_preserve_group.set(configdir['preserve_group'])
                self.opt_preserve_permission.set(
                    configdir['preserve_permission'])
                self.opt_preserve_hardlink.set(configdir['preserve_hardlink'])
                self.opt_preserve_symlink.set(configdir['preserve_symlink'])
                self.opt_preserve_device.set(configdir['preserve_device'])
                self.opt_delete.set(configdir['skip_exist'])
                self.opt_skip_exist.set(configdir['skip_exist'])
                self.opt_non_incre.set(configdir['non_incre'])
                self.opt_additional.set(configdir['additional'])
                self.opt_description.delete('1.0', END)
                self.opt_description.insert(END,configdir['description'].decode('utf-8'))

            except:
                showerror('Rsync Gui', '加载失败，请选择正确的配置文件！！')
            else:
                showinfo('Rsync Gui', '配置加载成功！')

    def stor_config(self):
        savename = asksaveasfilename(filetypes=[('Config file', '*.pkl')])
        if not savename:
            return

        configdir = {}
        configdir['super_user'] = True if self.opt_super_user.get() else False
        configdir['recursive'] = True if self.opt_recursive.get() else False
        configdir[
            'preserve_time'] = True if self.opt_preserve_time.get() else False
        configdir['preserve_owner'] = True if self.opt_preserve_owner.get(
        ) else False
        configdir['preserve_group'] = True if self.opt_preserve_group.get(
        ) else False
        configdir[
            'preserve_permission'] = True if self.opt_preserve_permission.get(
            ) else False
        configdir[
            'preserve_hardlink'] = True if self.opt_preserve_hardlink.get(
            ) else False
        configdir['preserve_symlink'] = True if self.opt_preserve_symlink.get(
        ) else False
        configdir['preserve_device'] = True if self.opt_preserve_device.get(
        ) else False
        configdir['delete'] = True if self.opt_delete.get() else False
        configdir['skip_exist'] = True if self.opt_skip_exist.get() else False
        configdir['non_incre'] = True if self.opt_non_incre.get() else False
        configdir['additional'] = self.opt_additional.get().strip()
        configdir['description'] = self.opt_description.get('1.0', END + '-1c').encode('utf-8')
        try:
            file = open(savename, 'wb')
            pickle.dump(configdir, file)
            file.close()
        except:
            showerror('Rsync Gui', '无法写入文件' + savename)
        else:
            showinfo('Rsync Gui', '保存成功！')

    def about(self):
        helptext = """A tkinter GUI for rsync
选项部分
    递归复制子文件 -r 
    保留时间  -t  (保留文件的修改时间)
    保留权限  -p
    保留所属者 -o (需要超级用户权限)
    保留所属组 -g
    保留硬连接 -H
    保留符号连接 -l
    保留设备文件 -D
    删除不在源中的文件 --delete (删除目标文件夹中不在源文件夹中的文件)
    跳过已存在的文件  --ignore-existing (忽略哪些已在目标文件夹中的文件，如果某次复制中断，要继续时选择这个选项将会快速跳过已复制的那些文件)
    以超级用户运行  (以superroot运行rsync)
    开始前完成扫描 --no-i-r (rsync 在开始前将扫描所有要复制的文件，这会提高显示进度的准确性但是如果文件较多会占用较长时间)
    附加选项  (可以指定任意合法的rsync参数)
    
    默认使用了选项 -h(使显示的数据更友好) 以及 --progress(输出复制的过程)

进度部分
    当前操作显示的是正在复制的文件名称以及进度，当前的速度和已复制的大小
    总进度显示三个数字   已复制的文件数目/已复制的文件以及文件夹数目/总的文件及文件夹数目
        如果没有选择 开始前完成扫描 选项，总数目会在过程中持续增长
    下方显示的是命令的正常输出

错误部分
    命令的错误输出，显示复制失败的文件，失败原因以命令的终止原因
"""
        a = Toplevel()
        st_help = ScrolledText(a, font=('Helvetica', 12))
        st_help.pack(expand=True, fill=BOTH)
        st_help.insert(END, helptext)
        st_help.config(state=DISABLED)

    def rsync_info(self):
        p = subprocess.Popen(
            'rsync --version', shell=True, stdout=subprocess.PIPE)
        info_text = p.stdout.read()
        showinfo('rsync info', info_text)

    def rsync_man(self):
        p = subprocess.Popen('man rsync', shell=True, stdout=subprocess.PIPE)
        info_text = p.stdout.read()

        tl = Toplevel()
        tl.title('rsync manual')
        st_help = ScrolledText(tl, font=('Helvetica', 12))
        st_help.pack(expand=True, fill=BOTH)
        st_help.insert(END, info_text)
        st_help.config(state=DISABLED)




if __name__ == '__main__':
    if sys.platform[:3] == 'win':
        showerror(
            'Error',
            'Rsync can only run on unix/linux, your platform is ' + sys.platform)
        sys.exit(1)
    logging.basicConfig(level=logging.INFO)
    Application()