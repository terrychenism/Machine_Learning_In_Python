function deepdream_demo
if exist('../+caffe', 'dir')
  addpath('..');
else
  error('Please run this demo from caffe/matlab/demo');
end
caffe.reset_all();
caffe.set_mode_gpu();
gpu_id = 0;
caffe.set_device(gpu_id);
mean_pix =  [104.0, 116.0, 122.0];


net_model = 'deploy.prototxt';
net_weights = 'bvlc_googlenet.caffemodel';

net = caffe.Net(net_model,net_weights,'test');
im = single(imread('sky1024px.jpg'));
[H, W, C] = size(im);
blob_index = net.name2blob_index('data');
net.blob_vec(blob_index).reshape([ H  W 3 1])
% im = randn(size(im))*100; % use random noise
src = deepdream(net, im, 15);
images = src{1};
im_data = images(:, :, [3, 2, 1]); 


for c = 1:3
    im_data(:, :, c) = im_data(:, :, c) + mean_pix(c);
end

imshow(uint8(im_data));

end


function octaves = deepdream(net, base_img, iter)
    lr = 1;
    lambda = 0.1;
    step_size = 1.5;
    use_cv_norm = 0;
    octaves = {preprocess(base_img)};
    for i = 1:iter
%         dst = net.forward(octaves);
%         g = net.backward(dst);
%         g = g{1};

        net.blobs('data').set_data(octaves{1});
        net.forward_prefilled();
        dst = net.blobs('inception_4c/output').get_data();
        
        net.blobs('inception_4c/output').set_diff(dst);
        net.backward_prefilled();
        g = net.blobs('data').get_diff();
        
        octaves{1} = octaves{1} +  step_size/mean(abs(g(:))) * g;
        if use_cv_norm
            I = octaves{1};
            Gx = smoothL1(I(2:end-1,:,:) - I(1:end-2,:,:)) - smoothL1(I(3:end,:,:) - I(2:end-1,:,:));
            Gx = [smoothL1(I(1,:,:) - I(2,:,:)); Gx; smoothL1(I(end,:,:) - I(end-1,:,:))];
            Gy = smoothL1(I(:,2:end-1,:) - I(:,1:end-2,:)) - smoothL1(I(:,3:end,:) - I(:,2:end-1,:));
            Gy = [smoothL1(I(:,1,:) - I(:,2,:)) Gy smoothL1(I(:,end,:) - I(:,end-1,:))];
            octaves{1} = I + lr * lambda * (Gx + Gy);
        end
    end
    
end



function crops_data = preprocess(im)
    [h,w,c] = size(im);
    mean_pix =  [104.0, 116.0, 122.0];
    im_data = im(:, :, [3 2 1]);  % permute channels from RGB to BGR
    im_data = single(im_data);  % convert from uint8 to single
    crops_data = zeros(h, w, 3, 1, 'single');
    for c = 1:3
        crops_data(:, :, c, :) = im_data(:,:,c) - mean_pix(c);
    end
end

