import os

TARGET_DIR = "./user/target/bin/"

if __name__ == '__main__':
    f = open("os/link_app.S", mode="w")
    apps = os.listdir(TARGET_DIR)
    apps.sort()
    # .quad 2 定义了一个 8 字节（64 位）长的变量并初始化为 2
    f.write(
'''    .align 4
    .section .data
    .global _app_num
_app_num:
    .quad {}
'''.format(len(apps))
    )
    # 所有文件起始地址和结束地址
    # 定义了一个 8 字节的值，并将其初始化为其他符号的地址
    #  app_0_start 的地址（即 app_0_start 标签的值，可能是一个内存地址）作为 64 位值存储在数据段中。
    # 换句话说，它把 app_0_start 的地址写入内存，存储在一个 8 字节的位置
    for (idx, _) in enumerate(apps):
        f.write('    .quad app_{}_start\n'.format(idx))
    f.write('    .quad app_{}_end\n'.format(len(apps) - 1))

    f.write(
'''
    .global _app_names
_app_names:
''');

    # 定义字符串
    for app in apps:
        f.write("   .string \"" + app + "\"\n")

    # .incbin 意味着这个二进制文件的内容将直接放在内存中
    for (idx, app) in enumerate(apps):
        f.write(
'''
    .section .data.app{0}
    .global app_{0}_start
app_{0}_start:
    .incbin "{1}"
'''.format(idx, TARGET_DIR + app)
        )
    f.write('app_{}_end:\n\n'.format(len(apps) - 1))
    f.close()