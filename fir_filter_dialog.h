/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2018 - 2023 Teunis van Beelen
*
* Email: teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/



#ifndef ADD_FIR_FILTERFORM1_H
#define ADD_FIR_FILTERFORM1_H



#include "qt_headers.h"

#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "mainwindow.h"
#include "utils.h"

#include "filt/fir_filter.h"
#include "filt/remez.h"
#include "filtercurve.h"


#define FIR_FILTER_MAX_TAPS           (5000)
#define FIR_FILTER_MAX_BUFSZ        (100000)


class UI_Mainwindow;


class UI_FIRFilterRZ_thread : public QThread
{
public:

struct rz_params_struct
{
  int band_num;
  double sf;
  double start_hz[32];
  double stop_hz[32];
  double gain[32];
  double ripple[32];
  double *f_taps;
  int tap_num;
}rz_params;

UI_FIRFilterRZ_thread();
~UI_FIRFilterRZ_thread();

private:

void run();

};


class UI_FIRFilterDialog : public QObject
{
  Q_OBJECT

public:
  UI_FIRFilterDialog(char *recent_dir=NULL, char *save_dir=NULL, QWidget *parent=0);
  ~UI_FIRFilterDialog();

  UI_Mainwindow *mainwindow;


private:

QDialog        *firfilterdialog;

QPushButton    *CancelButton,
               *ApplyButton,
               *helpButton,
               *fileButton,
               *add_passband_button,
               *add_stopband_button,
               *design_button;

QComboBox      *predefined_cbox;

QListWidget    *list;

QPlainTextEdit *textEdit;

QLabel         *listlabel,
               *varsLabel,
               *freqresplabel;

QSpinBox       *db_min_spinbox,
               *db_max_spinbox;

QDoubleSpinBox *sf_spinbox;

QCheckBox      *plotphasecheckbox;

QTableWidget *param_table;

FilterCurve    *curve1;

UI_FIRFilterRZ_thread *thrd;

int n_taps,
    db_max_i,
    db_min_i,
    plot_phase;

double taps[FIR_FILTER_MAX_TAPS],
       *z,
       *deg;

char textbuf[FIR_FILTER_MAX_BUFSZ + 64],
     *recent_opendir,
     *recent_savedir;

int add_table_row(void);

private slots:

void ApplyButtonClicked();
void check_text();
void helpbuttonpressed();
void filebuttonpressed();
void db_min_spinbox_changed(int);
void db_max_spinbox_changed(int);
void plotphasecheckbox_changed(int);
void selected_signals_changed();
void add_passband_button_pressed();
void add_stopband_button_pressed();
void design_button_pressed();
void tabel_delete_pressed(int, int);
void predefined_cbox_changed(int);
void paramtable_rowcount_changed();

};



#endif // ADD_FIR_FILTERFORM1_H








