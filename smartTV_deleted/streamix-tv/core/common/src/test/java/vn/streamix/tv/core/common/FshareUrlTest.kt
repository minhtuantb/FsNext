package vn.streamix.tv.core.common

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class FshareUrlTest {

    @Test
    fun `normalize http to https`() {
        assertEquals(
            "https://www.fshare.vn/file/ABC",
            FshareUrl.normalize("http://www.fshare.vn/file/ABC"),
        )
    }

    @Test
    fun `normalize adds www prefix`() {
        assertEquals(
            "https://www.fshare.vn/file/ABC",
            FshareUrl.normalize("https://fshare.vn/file/ABC"),
        )
    }

    @Test
    fun `normalize strips fragment`() {
        assertEquals(
            "https://www.fshare.vn/file/ABC",
            FshareUrl.normalize("https://www.fshare.vn/file/ABC#section1"),
        )
    }

    @Test
    fun `normalize keeps query`() {
        assertEquals(
            "https://www.fshare.vn/file/ABC?token=xyz",
            FshareUrl.normalize("https://www.fshare.vn/file/ABC?token=xyz"),
        )
    }

    @Test
    fun `normalize returns null for non-fshare`() {
        assertNull(FshareUrl.normalize("https://google.com"))
    }

    @Test
    fun `normalize returns null for blank`() {
        assertNull(FshareUrl.normalize(""))
        assertNull(FshareUrl.normalize("   "))
        assertNull(FshareUrl.normalize(null))
    }

    @Test
    fun `extractLinkcode from file URL`() {
        assertEquals("ABC123", FshareUrl.extractLinkcode("https://www.fshare.vn/file/ABC123"))
        assertEquals("ABC123", FshareUrl.extractLinkcode("https://www.fshare.vn/file/ABC123?token=x"))
    }

    @Test
    fun `extractLinkcode from folder URL`() {
        assertEquals("XYZ789", FshareUrl.extractLinkcode("https://www.fshare.vn/folder/XYZ789"))
    }

    @Test
    fun `extractLinkcode returns null for invalid`() {
        assertNull(FshareUrl.extractLinkcode("https://google.com"))
        assertNull(FshareUrl.extractLinkcode(""))
        assertNull(FshareUrl.extractLinkcode(null))
    }

    @Test
    fun `isFolderUrl positive`() {
        assertTrue(FshareUrl.isFolderUrl("https://www.fshare.vn/folder/ABC"))
        assertFalse(FshareUrl.isFolderUrl("https://www.fshare.vn/file/ABC"))
        assertFalse(FshareUrl.isFolderUrl(""))
        assertFalse(FshareUrl.isFolderUrl(null))
    }

    @Test
    fun `isFileUrl positive`() {
        assertTrue(FshareUrl.isFileUrl("https://www.fshare.vn/file/ABC"))
        assertFalse(FshareUrl.isFileUrl("https://www.fshare.vn/folder/ABC"))
    }

    @Test
    fun `stripQuery removes query string`() {
        assertEquals(
            "https://www.fshare.vn/file/ABC",
            FshareUrl.stripQuery("https://www.fshare.vn/file/ABC?token=xyz&abc=1"),
        )
    }

    @Test
    fun `stripQuery preserves URL without query`() {
        assertEquals(
            "https://www.fshare.vn/file/ABC",
            FshareUrl.stripQuery("https://www.fshare.vn/file/ABC"),
        )
    }

    @Test
    fun `folderUrl builds canonical URL`() {
        assertEquals("https://www.fshare.vn/folder/ABC", FshareUrl.folderUrl("ABC"))
    }

    @Test
    fun `fileUrl builds canonical URL`() {
        assertEquals("https://www.fshare.vn/file/ABC", FshareUrl.fileUrl("ABC"))
    }
}
