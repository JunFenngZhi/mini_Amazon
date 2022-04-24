# Generated by Django 2.2.28 on 2022-04-24 01:08

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='Item',
            fields=[
                ('item_id', models.IntegerField(primary_key=True, serialize=False)),
                ('name', models.CharField(max_length=100)),
                ('description', models.CharField(max_length=100, null=True, unique=True)),
                ('price', models.FloatField(max_length=1000)),
                ('catalog', models.CharField(max_length=100)),
            ],
            options={
                'db_table': 'item',
            },
        ),
        migrations.CreateModel(
            name='ShoppingCart',
            fields=[
                ('ShoppingCartID', models.AutoField(primary_key=True, serialize=False)),
                ('userName', models.CharField(max_length=50, null=True)),
                ('name', models.CharField(max_length=50, null=True)),
                ('amount', models.IntegerField()),
                ('itemID', models.IntegerField()),
                ('item_price', models.FloatField(max_length=100)),
            ],
            options={
                'db_table': 'shoppingcart',
            },
        ),
        migrations.CreateModel(
            name='UserProfile',
            fields=[
                ('userName', models.CharField(max_length=50, primary_key=True, serialize=False)),
                ('addrX', models.CharField(max_length=10, null=True)),
                ('addrY', models.CharField(max_length=10, null=True)),
                ('upsID', models.CharField(max_length=10, null=True)),
            ],
        ),
        migrations.CreateModel(
            name='Order',
            fields=[
                ('pack_id', models.AutoField(primary_key=True, serialize=False)),
                ('customer_name', models.CharField(max_length=50)),
                ('addr_x', models.IntegerField()),
                ('addr_y', models.IntegerField()),
                ('time', models.TimeField()),
                ('status', models.CharField(choices=[('PACKING', 'packing'), ('PACKED', 'packed'), ('LOADING', 'loading'), ('LOADED', 'loaded'), ('DELIVERING', 'delivering'), ('DELIVERED', 'delivered')], default='packing', max_length=50)),
                ('amount', models.IntegerField()),
                ('ups_id', models.IntegerField()),
                ('wh_id', models.IntegerField()),
                ('price', models.FloatField(max_length=1000)),
                ('version', models.IntegerField()),
                ('item', models.ForeignKey(null=True, on_delete=django.db.models.deletion.SET_NULL, to='amazon.Item')),
            ],
            options={
                'db_table': 'orders',
            },
        ),
        migrations.CreateModel(
            name='Inventory',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('item_amount', models.IntegerField()),
                ('wh_id', models.IntegerField()),
                ('version', models.IntegerField()),
                ('item', models.ForeignKey(null=True, on_delete=django.db.models.deletion.SET_NULL, to='amazon.Item')),
            ],
            options={
                'db_table': 'inventory',
                'unique_together': {('item', 'wh_id')},
            },
        ),
    ]
