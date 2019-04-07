#!/usr/bin/python3
from tkinter import *
from tkinter.messagebox import showinfo, showerror
from tkinter.simpledialog import askinteger, askstring
import random, sys

COLOR = ("#cdc0b0", "#EEE4DA", "#EDE0C8", "#F2B179", "#F59563", "#F67C5F",
         "#F65E3B", "#EDCF72", "#EDCC61", "#EDC850", "#EDC53F", "#EDC22E",
         "#3E3933")
BACKGROUNDCOLOR = '#faf8ef'
BOARDCONTAINERCOLOR = '#bbada0'


class game():
    def __init__(self, root, degree):
        self.mark = 0
        self.degree = degree
        self.board = []
        self.tile = []
        self.mark_label = Label(
            root,
            text='Score:  0',
            font=("Helvetica", 20, "bold"),
            bd=3,
            bg=BACKGROUNDCOLOR)
        self.mark_label.pack(side=TOP, fill=X)

        self.board_frame = Frame(root, padx=15, pady=20, bg=BACKGROUNDCOLOR)
        self.board_frame.pack(fill=BOTH, expand=True)
        self.tileframe = Frame(self.board_frame, padx=5, pady=5, bg=BOARDCONTAINERCOLOR)
        self.tileframe.pack(fill=BOTH, expand=YES)

        for i in range(self.degree):
            self.board.append([])
            self.tile.append([])
            self.tileframe.rowconfigure(i, weight=1)
            self.tileframe.columnconfigure(i, weight=1, uniform='same')
            for j in range(self.degree):
                aframe = Frame(self.tileframe, padx=5, pady=5, bg=BOARDCONTAINERCOLOR)
                tile = Button(
                    aframe,
                    text=' ',
                    font=("Arial", 30, "bold"),
                    bd=10,
                    state=DISABLED,
                    relief=FLAT,
                    disabledforeground='black')
                tile.pack(fill=BOTH, expand=YES)
                aframe.grid(row=i, column=j, sticky=NSEW)
                self.board[i].append(0)
                self.tile[i].append(tile)

        root.bind('<Up>', self.onUp)
        root.bind('<Down>', self.onDown)
        root.bind('<Left>', self.onLeft)
        root.bind('<Right>', self.onRight)
        self.addramdom(self.getempty())

    def destroy(self):
        self.mark_label.destroy()
        self.board_frame.destroy()

    def onUp(self, event):
        afmove = self.move_up()
        if afmove:
            afrows, mark = afmove
            self.mark += mark
            for i in range(self.degree):
                if afrows[i]:
                    afrows[i].reverse()
                    for j in range(self.degree):
                        self.board[j][i] = afrows[i][j]
            self.aftermove()

    def onDown(self, event):
        afmove = self.move_down()
        if afmove:
            afrows, mark = afmove
            self.mark += mark
            for i in range(self.degree):
                if afrows[i]:
                    for j in range(self.degree):
                        self.board[j][i] = afrows[i][j]
            self.aftermove()

    def onLeft(self, event):
        afmove = self.move_left()
        if afmove:
            afrows, mark = afmove
            self.mark += mark
            for i in range(self.degree):
                if afrows[i]:
                    afrows[i].reverse()
                    for j in range(self.degree):
                        self.board[i][j] = afrows[i][j]
            self.aftermove()

    def onRight(self, event):
        afmove = self.move_right()
        if afmove:
            afrows, mark = afmove
            self.mark += mark
            for i in range(self.degree):
                if afrows[i]:
                    for j in range(self.degree):
                        self.board[i][j] = afrows[i][j]
            self.aftermove()

    def aftermove(self):
        emp = self.getempty()
        self.addramdom(emp)
        if self.isover():
            name = askstring(
                'Game Over',
                'Your score is ' + str(self.mark) + ' Inpur your Name!')
            if name:
                try:
                    records = open('record.dat', 'r').readlines()
                except:
                    records = []

                records.append((name + ' ' + str(self.mark) + '\n'))
                print(records)
                records.sort(key=lambda rec: int(rec.split()[1]), reverse=True)
                try:
                    file = open('record.dat', 'w')
                    for line in records:
                        file.write(line)
                    file.close()
                except:
                    print('save err!')
            self.showrecord()

        else:
            print(self.mark)
            self.fresh()

    def makeInfoRow(self, parent, name, score, width=10):
        row = Frame(parent)
        row.config(padx=10, pady=5)
        name = Label(
            row,
            text=name,
            width=width,
            font=('Helvetica', 12),
            relief=FLAT,
            anchor='w')
        info = Label(
            row, text=score, relief=FLAT, anchor='w', font=('Helvetica', 12))

        row.pack(fill=X)
        name.pack(side=LEFT)
        info.pack(fill=X)

    def showrecord(self):
        try:
            recs = open('record.dat', 'r').readlines()
        except:
            print('record file not found!')
        else:
            recwin = Toplevel()
            recwin.title('Records')
            i = 1
            for rec in recs:
                print(rec)
                name, score = rec.split()
                self.makeInfoRow(recwin, str(i) + '. ' + name, str(score))
                i += 1

    def move_right(self):
        bfrows = []
        for i in range(self.degree):
            bfrows.append([])
            for j in range(self.degree):
                bfrows[i].append(self.board[i][j])
        print(bfrows)
        return self.move_all_rows(bfrows)

    def move_left(self):
        bfrows = []
        for i in range(self.degree):
            bfrows.append([])
            l = list(range(self.degree))
            l.reverse()
            for j in l:
                bfrows[i].append(self.board[i][j])
        print(bfrows)
        return self.move_all_rows(bfrows)

    def move_down(self):
        bfrows = []
        for i in range(self.degree):
            bfrows.append([])
            for j in range(self.degree):
                bfrows[i].append(self.board[j][i])
        print(bfrows)
        return self.move_all_rows(bfrows)

    def move_up(self):
        bfrows = []
        for i in range(self.degree):
            bfrows.append([])
            l = list(range(self.degree))
            l.reverse()
            for j in l:
                bfrows[i].append(self.board[j][i])
        print(bfrows)
        return self.move_all_rows(bfrows)

    def mov_single_row(self, row):
        mark = 0
        seq = []
        for i in range(self.degree):
            if row[i]:
                seq.append(row[i])
        procpos = len(seq) - 1
        res = [0] * self.degree
        donepos = self.degree - 1
        while procpos >= 0:
            if procpos == 0:
                res[donepos] = seq[procpos]
                procpos -= 1
            else:
                if seq[procpos - 1] == seq[procpos]:
                    res[donepos] = seq[procpos] + 1
                    mark += 2**(seq[procpos] + 1)
                    procpos -= 2
                else:
                    res[donepos] = seq[procpos]
                    procpos -= 1
            donepos -= 1

        for i in range(self.degree):
            if not row[i] == res[i]:
                return (res, mark)
        return None

    def move_all_rows(self, rows):
        res = []
        mark = 0
        for i in range(self.degree):
            bfrow = rows[i]
            afmov = self.mov_single_row(bfrow)
            if afmov:
                afrwo = afmov[0]
                mark += afmov[1]
                res.append(afrwo)
            else:
                res.append(None)
        for i in range(self.degree):
            if res[i]:
                return (res, mark)
        return None

    def isover(self):
        if self.getempty():
            return False
        if self.move_down():
            return False
        if self.move_up():
            return False
        if self.move_left():
            return False
        if self.move_right():
            return False
        return True

    def getempty(self):
        emp = []
        for i in range(self.degree):
            for j in range(self.degree):
                if not self.board[i][j]:
                    emp.append((i, j))
        return emp

    def addramdom(self, emp):
        choice = random.choice(emp)
        self.board[choice[0]][choice[1]] = random.choice(
            [1, 1, 1, 1, 1, 1, 1, 1, 1, 2])
        self.fresh()

    def fresh(self):
        for i in range(self.degree):
            for j in range(self.degree):
                if self.board[i][j]:
                    clr = COLOR[self.board[i][j]]
                    self.tile[i][j].config(
                        text=str(2**self.board[i][j]), bg=clr)
                    if self.board[i][j] >= 7:
                        self.tile[i][j].config(font=("Arial", 25, "bold"))
                    elif self.board[i][j] >= 10:
                        self.tile[i][j].config(font=("Arial", 20, "bold"))
                    else:
                        self.tile[i][j].config(font=("Arial", 30, "bold"))
                else:
                    self.tile[i][j].config(text=' ', bg=COLOR[0])
                    self.tile[i][j].config(font=("Arial", 30, "bold"))
        self.mark_label.config(text='Score:  ' + str(self.mark))


class app():
    helptext = 'Tkinter programe 2048'

    def __init__(self, degree):
        self.root = Tk()
        self.root.geometry('570x620')
        self.root.title('2048')
        btnfrm = Frame(self.root, bg=BACKGROUNDCOLOR, padx=15, pady=4)
        btnfrm.pack(side=TOP, fill=X)
        Button(
            btnfrm,
            text='Restart',
            command=self.restart,
            bg=BACKGROUNDCOLOR,
            relief=FLAT).pack(side=LEFT)
        Button(
            btnfrm,
            text='Record',
            command=self.record,
            bg=BACKGROUNDCOLOR,
            relief=FLAT).pack(side=LEFT)
        Button(
            btnfrm,
            text='Help',
            command=self.help,
            bg=BACKGROUNDCOLOR,
            relief=FLAT).pack(side=RIGHT)
        self.game = game(self.root, degree)
        mainloop()

    def restart(self):
        degree = askinteger('Choose a size', 'Input the size of board')
        if degree > 1:
            self.game.destroy()
            self.game = game(self.root, degree)
        else:
            showerror('Wrong!','size must a number bigger than 2')

    def help(self):
        showinfo('2048', self.helptext)

    def record(self):
        self.game.showrecord()


if __name__ == '__main__':
    degree = 4
    if len(sys.argv) == 2:
        try:
            degree = int(sys.argv[1])
        except:
            print('bad arg!')
    app(degree)
