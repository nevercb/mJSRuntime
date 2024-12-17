console.log("Hello, QuickJS!");

// 测试 setTimeout
setTimeout(() => {
    console.log("Timeout 1 executed after 1000ms!");
}, 1000);

setTimeout(() => {
    console.log("Timeout 2 executed after 2000ms!");
}, 2000);

// 测试 Promise
const promise = new Promise((resolve, reject) => {
    console.log("Promise started");
    setTimeout(() => {
        resolve("Promise resolved after 1500ms!");
    }, 1500);
});

promise.then((message) => {
    console.log(message);
}).catch((err) => {
    console.log("Promise rejected:", err);
});
