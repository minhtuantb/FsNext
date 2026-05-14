# API: FileOps - Favorites

---

## 1. Change Favorite

### Endpoint
```
POST /api/fileops/changeFavorite
```
**Auth:** Required

### Input
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `token` | string | Yes | CSRF |
| `items` | array | Yes | Array with one linkcode (`items[0]`) |
| `status` | int | Yes | 1=add/enable, 0=disable |

### Business Logic
```
1. Get file info and file ID
2. If status == 1:
   - If favorite exists: UPDATE status=1, modified=now
   - If not exists AND total < maxFavorite param: INSERT new favorite
   - If at max: 409 "Maximum N favorite"
3. If status == 0:
   - UPDATE existing favorite to status=0
```

### Data Interactions
| Table | Operation |
|-------|-----------|
| `userfile` (MySQL) | Read file info |
| `favorite` (MySQL) | SELECT/INSERT/UPDATE |

### Output
Success: `{"code": 200, "msg": "Change favorite successed"}`
Error: `{"code": 409, "msg": "Maximum {N} favorite"}` or `{"code": 409, "msg": "Change favorite failed"}`

---

## 2. Is Favorite

### Endpoint
```
GET /api/fileops/isFavorite?linkcode={linkcode}
```
**Auth:** Required

### Input
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `linkcode` | string | `''` | File linkcode |

### Logic
```
1. If empty or file not found: return isFavorite=false (graceful)
2. Check Favorite::checkExist(fileId, userId)
```

### Output
```json
{
  "success": true,
  "isFavorite": true,
  "linkcode": "ABCDEFGH",
  "msg": ""
}
```

---

## 3. Add Favorite (Multiple URLs)

### Endpoint
```
POST /api/fileops/addFavorite
```
**Auth:** Required

### Input
| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `token` | string | Yes | CSRF |
| `link` | string | Yes | Comma or semicolon-separated fshare URLs |

### URL Validation Regex
```
/(\/\S*)?(http|https)\:\/\/www.fshare.vn\/(file|folder)\/[a-zA-Z0-9]{10,16}(\/\S*)?/
```

### Logic
```
1. Parse and validate each URL → 409 if invalid
2. Resolve each to file_id
3. Check maxFavorite limit
4. UPDATE existing favorites to status=1; INSERT new ones
```

### Output
Success: `{"code": 200, "msg": "Add favorite successed"}`
Error: `{"code": 409, "msg": "Invalid URL / Max favorites / DB error"}`

---

## 4. List Favorites

### Endpoint
```
GET /api/fileops/listFavorite?ext={ext}
```
**Auth:** Required

### Input
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `ext` | string | `''` | Extension filter (same as list: video, music, or specific) |

### Logic
```
1. Get favorites via Favorite::getAllFavoriteByUserID(userId, extArray)
2. 404 if false
3. Separate into folders + files
4. Image files get thumbnail URLs (xs, l)
5. Return: folders + image files + other files
```

### Output
Array of favorite file/folder objects with optional thumbnail URLs.

### Errors
| Code | Condition |
|------|-----------|
| 400 | Invalid ext format |
| 404 | No favorites found |

### Data Interactions
| Table | Operation |
|-------|-----------|
| `favorite` (MySQL) | JOIN with `userfile` |
| `download_sessions` (MongoDB) | Image thumbnails |

---

## Source Reference

- Controller: `protected/controllers/api/FileopsController.php`
- Model: `protected/models/mysql/Favorite.php`
