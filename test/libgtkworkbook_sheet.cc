/*
  The GTKWorkbook Project <http://gtkworkbook.sourceforge.net/>
  Copyright (C) 2009 John Bellone, Jr. <jvb4@njit.edu>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PRACTICAL PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301 USA
*/
#include "SheetTest.h"

// Basic sanity checks to make sure the underlying fixture is working.
TEST_F (SheetTest, FixtureIsWorking) {
	ASSERT_TRUE (gtk_window != NULL);
	EXPECT_TRUE (workbook != NULL);
	EXPECT_TRUE (sheet->workbook != NULL);
}

// Sanity check to verify that the pointer to the associated Workbook
//	object is equal.
TEST_F (SheetTest, WorkbookPtrIsEqual) {
	EXPECT_EQ (sheet->workbook, workbook);	
}

// Make sure that the "focus" property of the sheet is correct.
TEST_F (SheetTest, SheetHasFocus) {
	EXPECT_TRUE (sheet->has_focus == TRUE);
	EXPECT_EQ (sheet, workbook->focus_sheet);

	Sheet * a = workbook->add_new_sheet (workbook, "Two", 5, 5);

	EXPECT_TRUE (sheet->has_focus == TRUE);
	EXPECT_TRUE (a->has_focus == FALSE);
	EXPECT_EQ (sheet, workbook->focus_sheet);
}

// The first sheet added inside of the fixture gains the UI "focus" and any additional
//	sheet must be switched by clicking on the tab inside of the GtkNotebook widget. Notices
//	only accumulate when a sheet is not in focus.
TEST_F (SheetTest, NoticeCountWorks) {
	Sheet * a = workbook->add_new_sheet (workbook, "two", 5, 5);

	sheet->set_cell (sheet, 0, 0, "0,0");
	sheet->set_cell (sheet, 1, 1, "1,1");
	sheet->set_cell (sheet, 2, 4, "2,4");

	EXPECT_TRUE (sheet->has_focus == TRUE);
	EXPECT_TRUE (sheet == workbook->focus_sheet);
	EXPECT_EQ (0, sheet->notices);

	a->set_cell (a, 0, 0, "0,0");
	a->set_cell (a, 1, 1, "1,1");
	a->set_cell (a, 2, 4, "2,4");

	EXPECT_TRUE (a->has_focus == FALSE);
	EXPECT_EQ (3, a->notices);
}

// This method verifies that the "set_cell" method does indeed change the textual
//	data for the underlying GtkSheet widget.
TEST_F (SheetTest, MethodSetCellWorks) {
	sheet->set_cell (sheet, 0, 0, "0,0");
	sheet->set_cell (sheet, 1, 1, "1,1");
	sheet->set_cell (sheet, 2, 4, "2,4");

	GtkSheet * gtksheet = GTK_SHEET (sheet->gtk_sheet);
	
	EXPECT_STREQ ("0,0", gtksheet->data[0][0]->text);
	EXPECT_STREQ ("1,1", gtksheet->data[1][1]->text);
	EXPECT_STREQ ("2,4", gtksheet->data[2][4]->text);

	sheet->set_cell (sheet, 0, 0, "2,4");

	EXPECT_STREQ ("2,4", gtksheet->data[0][0]->text);
}

// This just verifies that the "set_attention" method properly does its job.
TEST_F (SheetTest, MethodSetAttentionWorks) {
	EXPECT_EQ (0, sheet->attention);

	sheet->set_attention (sheet, 10);

	EXPECT_EQ (10, sheet->attention);

	sheet->set_attention (sheet, 25);

	EXPECT_EQ (25, sheet->attention);
}

// This test shows that when a range is "frozen" cell updates cannot be applied, and when
//	the range is "thawed" that updates will once again be active. 
TEST_F (SheetTest, MethodFreezeAndThawSelectionWorks) {
	GtkSheet * gtksheet = GTK_SHEET (sheet->gtk_sheet);
	gtksheet->range.row0 = gtksheet->range.rowi =
		gtksheet->range.col0 = gtksheet->range.coli = 0;

	// TODO(jb): Add tests for other areas of cell attributes that cannot be updated while
	//	the cell is frozen.
	
	sheet->set_cell (sheet, 0, 0, "Foo");

	EXPECT_STREQ ("Foo", gtksheet->data[0][0]->text);
	
	sheet->freeze_selection (sheet);

	sheet->set_cell (sheet, 0, 0, "Bar");
	sheet->set_cell (sheet, 1, 1, "Foo");  /* Outside of the Range */

	EXPECT_STRNE ("Bar", gtksheet->data[0][0]->text);
	EXPECT_STREQ ("Foo", gtksheet->data[0][0]->text);
	EXPECT_STREQ ("Foo", gtksheet->data[1][1]->text);

	sheet->thaw_selection (sheet);

	sheet->set_cell (sheet, 0, 0, "Bar");
	
	EXPECT_STRNE ("Foo", gtksheet->data[0][0]->text);
	EXPECT_STREQ ("Bar", gtksheet->data[0][0]->text);
	EXPECT_STREQ ("Foo", gtksheet->data[1][1]->text);
}

// This test checks to make sure that the Sheet object's "apply_cell" method will take
//	the parameters from a Cell object pointer and apply it to the GtkSheet widget.
TEST_F (SheetTest, MethodApplyCellWorks) {
	GtkSheet * gtksheet = GTK_SHEET (sheet->gtk_sheet);
	Cell * cell = cell_new();
	cell->row = cell->column = 0;
	
	sheet->set_cell (sheet, 0, 0, "Zero");

	EXPECT_STREQ ("Zero", gtksheet->data[0][0]->text);

	g_string_assign (cell->value, "One");

	sheet->apply_cell (sheet, cell);

	EXPECT_STREQ ("One", gtksheet->data[0][0]->text);

	cell->row = cell->column = 1;

	sheet->apply_cell (sheet, cell);

	EXPECT_STREQ ("One", gtksheet->data[0][0]->text);
	EXPECT_STREQ ("One", gtksheet->data[1][1]->text);
	
	cell->destroy (cell);
}

TEST_F (SheetTest, MethodGetRowWorks) {
	GtkSheet * gtksheet = GTK_SHEET (sheet->gtk_sheet);
	
	// We are going to need the same number of cell objects that a single row would have.
	Cell ** row = (Cell **)malloc (sizeof (Cell *) * gtksheet->maxcol);
	for (int ii = 0; ii < gtksheet->maxcol; ii++) {
		sheet->set_cell (sheet, 0, ii, "asdfkasdgjkasdf");
		row[ii] = cell_new();
	}
	
	sheet->get_row (sheet, 0, row, gtksheet->maxcol);
	
	// Tear everything down and make sure we don't leak any memory.
	for (int ii = 0; ii < gtksheet->maxcol; ii++) {
		EXPECT_STREQ ("asdfkasdgjkasdf", row[ii]->value->str) << "Assertion failure in column "<<ii;
		row[ii]->destroy (row[ii]);
	}
	free (row);
}

TEST_F (SheetTest, MethodApplyArrayWorks) {
	GtkSheet * gtksheet = GTK_SHEET (sheet->gtk_sheet);
	
	// We are going to need the same number of cell objects that a single row would have.
	Cell ** row = (Cell **)malloc (sizeof (Cell *) * gtksheet->maxcol);
	for (int ii = 0; ii < gtksheet->maxcol; ii++) {
		row[ii] = cell_new();
		row[ii]->set (row[ii], 0, ii, "asdfkasdgjkasdf");
		sheet->set_cell (sheet, 0, ii, ""); /* Columns need to be allocated inside of the widget. */
		EXPECT_STREQ ("", gtksheet->data[0][ii]->text) << "Assertion failure in column "<<ii;
	}

	sheet->apply_array (sheet, row, gtksheet->maxcol);

	for (int ii = 0; ii < gtksheet->maxcol; ii++) {
		EXPECT_STREQ ("asdfkasdgjkasdf", gtksheet->data[0][ii]->text) << "Assertion failure in column "<<ii;
		row[ii]->destroy (row[ii]);
	}
	free (row);
}

TEST_F (SheetTest, MethodApplyRowWorks) {
	GtkSheet * gtksheet = GTK_SHEET (sheet->gtk_sheet);

	for (int ii = 0; ii < gtksheet->maxcol; ii++) {
		sheet->set_cell (sheet, 0, ii, "");
		EXPECT_STREQ ("", gtksheet->data[0][ii]->text) << "Assertion failure in column "<<ii;
		g_string_assign (sheet->cells[0][ii]->value, "asdfkjglasdf");
	}

	sheet->apply_row (sheet, 0);
	
	for (int ii = 0; ii < gtksheet->maxcol; ii++) {
		EXPECT_STREQ ("asdfkjglasdf", gtksheet->data[0][ii]->text) << "Assertion failure in column "<<ii;
	}
}
