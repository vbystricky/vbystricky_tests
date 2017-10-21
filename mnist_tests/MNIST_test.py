import os
os.environ['TF_CPP_MIN_LOG_LEVEL']='2'
import tensorflow as tf
from tensorflow.examples.tutorials.mnist import input_data
import tensorflow.contrib.slim as slim
import numpy as np

BATCH_SIZE = 100
MAX_EPOCH  = 30

def simple_model(mnist):
    batch_per_epoch = mnist.train.images.shape[0] // BATCH_SIZE

    print("train images cnt: {}".format(mnist.train.images.shape[0]))
    print("train minibatch per epoch: {}".format(batch_per_epoch))


    x = tf.placeholder(tf.float32, [None, 784])
    y_ = tf.placeholder(tf.float32, [None, 10])
  
    y = slim.fully_connected(x, 10, activation_fn=None)

    cross_entropy = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=y_, logits=y))
    train_step = tf.train.AdamOptimizer().minimize(cross_entropy)

    correct_prediction = tf.equal(tf.argmax(y, 1), tf.argmax(y_, 1))
    accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))

    sess = tf.InteractiveSession()
    tf.global_variables_initializer().run()
    accuracy_array = np.zeros([MAX_EPOCH])
    for epoch in range(MAX_EPOCH):
        for _ in range(batch_per_epoch):
            batch_xs, batch_ys = mnist.train.next_batch(BATCH_SIZE)
            sess.run(train_step, feed_dict={x: batch_xs, y_: batch_ys})
        epoch_accuracy = sess.run(accuracy, feed_dict={x: mnist.test.images, y_: mnist.test.labels})
        accuracy_array[epoch] = epoch_accuracy 
        print("Epoch {}, test accuracy {:.2f} ".format(epoch, epoch_accuracy * 100))

    ret = {}
    ret['mul_cnt'] = 784*10
    ret['coef_cnt'] = 784 * 10 + 10
    ret['max_acc'] = np.max(accuracy_array)
    ret['max_acc_epoch'] = np.argmax(accuracy_array)
    return ret

def hide_levels_model(mnist, h_dims = [512]):
    batch_per_epoch = mnist.train.images.shape[0] // BATCH_SIZE

    print("hide_levels_model {}".format(h_dims))
    print("train images cnt: {}".format(mnist.train.images.shape[0]))
    print("train minibatch per epoch: {}".format(batch_per_epoch))


    x = tf.placeholder(tf.float32, [None, 784])
    y_ = tf.placeholder(tf.float32, [None, 10])
  
    global_step = tf.Variable(0, trainable=False)
    lr = tf.train.exponential_decay(0.001, global_step, 15 * batch_per_epoch, 0.5, staircase=True)

    net = x
    for h_dim in h_dims:
        net = slim.fully_connected(net, h_dim, activation_fn=tf.nn.relu)
    y = slim.fully_connected(net, 10, activation_fn=None)

    cross_entropy = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=y_, logits=y))
    train_step = tf.train.AdamOptimizer(lr).minimize(cross_entropy, global_step=global_step)

    correct_prediction = tf.equal(tf.argmax(y, 1), tf.argmax(y_, 1))
    accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))

    sess = tf.InteractiveSession()
    tf.global_variables_initializer().run()
    accuracy_array = np.zeros([MAX_EPOCH])
    for epoch in range(MAX_EPOCH):
        for _ in range(batch_per_epoch):
            batch_xs, batch_ys = mnist.train.next_batch(BATCH_SIZE)
            sess.run(train_step, feed_dict={x: batch_xs, y_: batch_ys})
        epoch_accuracy = sess.run(accuracy, feed_dict={x: mnist.test.images, y_: mnist.test.labels})
        accuracy_array[epoch] = epoch_accuracy 
        print("Epoch {}, test accuracy {:.2f} ".format(epoch, epoch_accuracy * 100))

    ret = {'mul_cnt' : 0, 'coef_cnt' : 0}
    last_dim = 784
    for h_dim in h_dims:
        ret['mul_cnt']  = ret['mul_cnt'] + last_dim * h_dim
        ret['coef_cnt'] = ret['coef_cnt'] + last_dim * h_dim + h_dim
        last_dim = h_dim
    ret['mul_cnt']  = ret['mul_cnt'] + last_dim * 10
    ret['coef_cnt'] = ret['coef_cnt'] + last_dim * 10 + 10

    ret['max_acc'] = np.max(accuracy_array)
    ret['max_acc_epoch'] = np.argmax(accuracy_array)
    return ret

def cnn_model(mnist, simple = True):
    batch_per_epoch = mnist.train.images.shape[0] // BATCH_SIZE

    print("train images cnt: {}".format(mnist.train.images.shape[0]))
    print("train minibatch per epoch: {}".format(batch_per_epoch))

    x = tf.placeholder(tf.float32, [None, 784])
    y_ = tf.placeholder(tf.float32, [None, 10])
    keep_prob = tf.placeholder(tf.float32)

    x_image = tf.reshape(x, [-1, 28, 28, 1])
    with slim.arg_scope([slim.conv2d], padding='SAME', activation_fn=tf.nn.relu):
        conv1 = slim.conv2d(x_image, 20, [5, 5])
        pool1 = slim.max_pool2d(conv1, [2, 2])
        conv2 = slim.conv2d(pool1, 20, [5, 5])
        pool2 = slim.max_pool2d(conv2, [2, 2])
    if simple:
        y = slim.fully_connected(slim.flatten(pool2), 10, activation_fn=None)
    else:
        fc = slim.fully_connected(slim.flatten(pool2), 100, activation_fn=tf.nn.relu)
        fc = slim.dropout(fc, keep_prob)
        y = slim.fully_connected(fc, 10, activation_fn=None)
       
    cross_entropy = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=y_, logits=y))
    train_step = tf.train.AdamOptimizer().minimize(cross_entropy)

    correct_prediction = tf.reduce_sum(tf.cast(tf.equal(tf.argmax(y, 1), tf.argmax(y_, 1)), tf.int32))
    #accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))

    sess = tf.InteractiveSession()
    tf.global_variables_initializer().run()
    accuracy_array = np.zeros([MAX_EPOCH])
    for epoch in range(MAX_EPOCH):
        for _ in range(batch_per_epoch):
            batch_xs, batch_ys = mnist.train.next_batch(BATCH_SIZE)
            sess.run(train_step, feed_dict={x: batch_xs, y_: batch_ys, keep_prob: 0.5})

        test_batch_cnt = mnist.test.images.shape[0] // BATCH_SIZE
        test_correct_prediction = 0
        for i in range(test_batch_cnt):
            batch_xs = mnist.test.images[i*BATCH_SIZE: (i+1)*BATCH_SIZE, :]
            batch_ys = mnist.test.labels[i*BATCH_SIZE: (i+1)*BATCH_SIZE, :]
            batch_correct_prediction = sess.run(correct_prediction, feed_dict={x: batch_xs, y_: batch_ys, keep_prob: 1.0})
            test_correct_prediction = test_correct_prediction + batch_correct_prediction
        epoch_accuracy = test_correct_prediction / (test_batch_cnt * BATCH_SIZE)
        accuracy_array[epoch] = epoch_accuracy
        print("Epoch {}, test accuracy {:.2f} ".format(epoch, epoch_accuracy * 100))

    ret = {}
    if simple:
        ret['mul_cnt'] = 1*5*5*20*28*28 + 20*5*5*20*14*14 + 7*7*20*10
        ret['coef_cnt'] = (1*5*5*20 + 20) + (20*5*5*20 + 20) + (7*7*20*10 + 10)
    else:
        ret['mul_cnt'] = 1*5*5*20*28*28 + 20*5*5*20*14*14 + 7*7*20*100 + 100*10
        ret['coef_cnt'] = (1*5*5*20 + 20) + (20*5*5*20 + 20) + (7*7*20*100 + 100) + (100*10 + 10)
    ret['max_acc'] = np.max(accuracy_array)
    ret['max_acc_epoch'] = np.argmax(accuracy_array)
    return ret

def print_result(model_name, result):
    coeff  = "Coefficients count {}.".format(result['coef_cnt'])
    operat = "Multiplications count {}.".format(result['mul_cnt'])
    acc = "Maximum accuracy {} on epoch {}.".format(result['max_acc'] * 100, result['max_acc_epoch'])
    if 'mean_acc' in result:
        acc = acc + "Mean accuracy {} Median accuracy {}.".format(result['mean_acc'] * 100, result['median_acc'] * 100)
    msg = model_name + ". " + coeff + " " + operat + " " + acc
    print(msg)

def some_attempts(mnist, model, max_attempts):
    model_acc_max = np.zeros([max_attempts])
    for att in range(max_attempts):
        model_res = model(mnist)
        model_acc_max[att] = model_res['max_acc']
    print(model_acc_max)
    model_res['mean_acc'] = np.mean(model_acc_max)
    model_res['median_acc'] = np.median(model_acc_max)
    return model_res

def test(mnist, models, max_attempts):
    for model in models:
        model['result'] = some_attempts(mnist, model['func'], max_attempts)
        print_result(model['name'], model['result'])
    print()
    for model in models:
        print_result(model['name'], model['result'])

mnist = input_data.read_data_sets('./MNIST_data', one_hot=True)

models = [
    {'name' : "Simple model",            'func' : simple_model},
    {'name' : "Hide level 128 neurons",  'func' : lambda x: hide_levels_model(x, [128])},
    {'name' : "Hide level 256 neurons",  'func' : lambda x: hide_levels_model(x, [256])},
    {'name' : "Hide level 512 neurons",  'func' : lambda x: hide_levels_model(x, [512])},
    {'name' : "Hide level 1024 neurons", 'func' : lambda x: hide_levels_model(x, [1024])},
    {'name' : "Hide level 2046 neurons", 'func' : lambda x: hide_levels_model(x, [2046])},
    {'name' : "CNN simple model",        'func' : cnn_model},
    {'name' : "CNN model",               'func' : lambda x: cnn_model(x, simple = False)},
    ]
test(mnist, models, 10)

