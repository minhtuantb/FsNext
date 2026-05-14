package vn.streamix.tv.core.common

import org.junit.Assert.assertEquals
import org.junit.Test

class CsvParserTest {

    @Test
    fun `parse simple csv`() {
        val csv = "a,b,c\n1,2,3\n4,5,6"
        val rows = CsvParser.parse(csv)
        assertEquals(3, rows.size)
        assertEquals(listOf("a", "b", "c"), rows[0])
        assertEquals(listOf("1", "2", "3"), rows[1])
        assertEquals(listOf("4", "5", "6"), rows[2])
    }

    @Test
    fun `parse with CRLF`() {
        val csv = "a,b\r\n1,2\r\n"
        val rows = CsvParser.parse(csv)
        assertEquals(2, rows.size)
        assertEquals(listOf("a", "b"), rows[0])
        assertEquals(listOf("1", "2"), rows[1])
    }

    @Test
    fun `parse quoted field with comma`() {
        val csv = """name,url
"Hello, World","https://www.fshare.vn/file/ABC""""
        val rows = CsvParser.parse(csv)
        assertEquals(2, rows.size)
        assertEquals(listOf("Hello, World", "https://www.fshare.vn/file/ABC"), rows[1])
    }

    @Test
    fun `parse escaped quotes inside quoted field`() {
        val csv = """label,url
"He said ""hi""","x""""
        val rows = CsvParser.parse(csv)
        assertEquals(listOf("He said \"hi\"", "x"), rows[1])
    }

    @Test
    fun `parse quoted field with newline`() {
        val csv = """a,b
"line1
line2",end"""
        val rows = CsvParser.parse(csv)
        assertEquals(2, rows.size)
        assertEquals(listOf("line1\nline2", "end"), rows[1])
    }

    @Test
    fun `parse empty input`() {
        val rows = CsvParser.parse("")
        assertEquals(0, rows.size)
    }

    @Test
    fun `parse trailing newline does not produce empty row`() {
        val csv = "a,b\n1,2\n"
        val rows = CsvParser.parse(csv)
        assertEquals(2, rows.size)
    }

    @Test
    fun `parse no trailing newline includes last row`() {
        val csv = "a,b\n1,2"
        val rows = CsvParser.parse(csv)
        assertEquals(2, rows.size)
        assertEquals(listOf("1", "2"), rows[1])
    }

    @Test
    fun `parse single row no newline`() {
        val rows = CsvParser.parse("a,b,c")
        assertEquals(1, rows.size)
        assertEquals(listOf("a", "b", "c"), rows[0])
    }

    @Test
    fun `parse empty cells`() {
        val rows = CsvParser.parse(",,,")
        assertEquals(1, rows.size)
        assertEquals(listOf("", "", "", ""), rows[0])
    }
}
