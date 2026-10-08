#pragma once

/**
 * 控制台工具
 * DLL 注入到 GUI 进程后，调用 open() 弹出一个黑窗口，
 * 之后 printf / std::cout 的内容都会显示在那里
 */
namespace tools::console {
    /** 分配控制台并重定向 stdout/stderr */
    void open();

    /** 关闭控制台（一般不用主动调） */
    void close();
}
