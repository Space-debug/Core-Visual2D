from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    engine_path = LaunchConfiguration("engine_path")
    image_topic = LaunchConfiguration("image_topic")
    thread_num = LaunchConfiguration("thread_num")
    return LaunchDescription([
        DeclareLaunchArgument(
            "engine_path",
            default_value="",
            description="Path to TensorRT engine file (.engine)"),
        DeclareLaunchArgument(
            "image_topic",
            default_value="image_raw",
            description="Input image topic"),
        DeclareLaunchArgument(
            "thread_num",
            default_value="1",
            description="Number of inference worker threads"),
        Node(
            package="visual2d_infer",
            executable="visual2d_node",
            name="visual2d_infer",
            output="screen",
            parameters=[{
                "engine_path": engine_path,
                "image_topic": image_topic,
                "thread_num": thread_num,
            }],
        ),
    ])
