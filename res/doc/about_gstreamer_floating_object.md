# GStreamer 对象引用计数
所有新创建的对象都设置了 `FLOATING`（浮动） 标志。这意味着该对象尚未被除持有该对象引用的人之外的任何人拥有或管理。处于此状态的对象引用计数为 1。

各种对象方法可以接管另一个对象的所有权，这意味着在以对象 B 作为参数调用对象 A 的某个方法后，对象 B 变为了对象 A 的**唯一财产**。这意味着在方法调用之后，除非你保留了该对象的**额外引用**，否则你**不再被允许访问该对象**。此类方法的一个例子是 `_bin_add()` 方法。一旦在 Bin 中调用此函数，作为参数传递的元素即**归该 Bin 所有**，如果不先进行 _ref() 操作就将其添加到 Bin 中，你将**不再被允许访问它**。原因在于，在调用 _bin_add() 之后，销毁 Bin 同时也销毁了该元素。

接管对象的所有权是通过“sinking（下沉）”对象的过程实现的。如果设置了 `FLOATING` 标志，对象上的 `_sink()` 方法将减少该对象的引用计数。接管对象所有权的行为随后通过对该对象先进行 `_ref()` 调用、后进行 `_sink()` 调用来完成。

在初始化那些随后将被置于父级控制下的元素时，“float/sink”过程非常有用。浮动引用使对象保持存活，直到它被指定父级；而一旦对象被指定了父级，你就可以不再管它了。

## 原文

All new objects created have the `FLOATING` flag set. This means that the object is not owned or managed yet by anybody other than the one holding a reference to the object. The object in this state has a reference count of 1.

Various object methods can take ownership of another object, this means that after calling a method on object A with an object B as an argument, the object B is made **sole property** of object A. This means that after the method call you are **not allowed** to access the object anymore unless you keep an **extra reference** to the object. An example of such a method is the `_bin_add()` method. As soon as this function is called in a Bin, the element passed as an argument is owned by the bin and you are not allowed to access it anymore without taking a `_ref()` before adding it to the bin. The reason being that after the `_bin_add()` call disposing the bin also destroys the element.

Taking ownership of an object happens through the process of "sinking" the object. the `_sink()` method on an object will decrease the refcount of the object if the `FLOATING` flag is set. The act of taking ownership of an object is then performed as a `_ref()` followed by a `_sink()` call on the object.

The float/sink process is very useful when initializing elements that will then be placed under control of a parent. The floating ref keeps the object alive until it is parented, and once the object is parented you can forget about it.

[原文地址](https://gstreamer.freedesktop.org/documentation/additional/design/MT-refcounting.html?gi-language=c#refcounting1)

