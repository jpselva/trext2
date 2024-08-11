# t-rext2

Implementação simplificada do sistema de arquivos ext2

## Testes

Para rodar os testes, execute:

```
$ make run-tests
```

## TODO

- [x] `ext2_mount(ext2_t* ext2, ext2_config_t cfg)`
- [x] `ext2_file_read(ext2_t* ext2, ext2_file_t* file, uint32_t size, void* buf)`
- [x] `ext2_file_seek(ext2_t* ext2, ext2_file_t* file, uint32_t offset)`
- [x] `uint32_t ext2_file_tell(ext2_t* ext2, ext2_file_t* file)`
- [ ] `ext2_dir_read(ext2_t* ext2, ext2_dir_t* dir, ext2_info* info)`
- [ ] `uint32_t ext2_dir_tell(ext2_t* ext2, ext2_dir_t* dir)`
- [ ] `ext2_dir_seek(ext2_t* ext2, ext2_dir_t* dir, uint32_t offset)`
- [ ] `ext2_file_open(ext2_t* ext2, const char* path, ext2_file_t file)`
- [ ] `ext2_file_write(ext2_t* ext2, ext2_file_t* file, uint32_t size, void* buf)`
- [ ] `ext2_file_truncate(ext2_t* ext2, ext2_file_t* file, uint32_t size)`
- [ ] `ext2_dir_open(ext2_t* ext2, const char* path, ext2_dir_t* dir)`
- [ ] `ext2_dir_mkdir(ext2_t* ext2, const char* path)`
- [ ] `ext2_rm(ext2_t* ext2, const char* path)`
- [ ] `ext2_rename(ext2_t ext2, const char* oldpath, const char* newpath)`
